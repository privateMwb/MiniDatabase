/**
 * @file WriteAheadLog.cpp
 * @brief MiniDB::Storage::WriteAheadLog implementation.
 *
 * Contains the implementation of the on-disk record framing, durable
 * append, and crash-recovery scan described in WriteAheadLog.h.
 */

// ============================================================
// Implementation for MiniDB::Storage::WriteAheadLog.
// ============================================================
//
//  Sections:
//   1. Checksum
//   2. Constructors & Destructor
//   3. Lifecycle
//   4. Append
//   5. Read
//   6. Truncate
//   7. Introspection
//
// ============================================================

#include <MiniDB/Storage/WriteAheadLog.h>

#include <array>
#include <cstring>
#include <utility>
#include <vector>

// Windows: included after every project header on purpose, so windows.h's
// macros can't leak into them. NOMINMAX/NOGDI keep min/max/ERROR out.
#if defined(_WIN32)
#include <filesystem> // std::filesystem::path (wide-char path for CreateFileW)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef NOGDI
#define NOGDI
#endif
// clang-format off
#include <windows.h>
#include <fcntl.h> // _O_RDWR, _O_BINARY
#include <io.h>    // _open_osfhandle, _get_osfhandle, _close, _chsize_s
// clang-format on
#elif !(defined(__unix__) || defined(__APPLE__))
#error "WriteAheadLog: unsupported platform (need POSIX or Windows)"
#endif

namespace MiniDB::Storage {

// ============================================================
//  Platform layer
//
//  Every OS-specific call the log makes goes through the seven sys*()
//  helpers below, so the recovery/append/truncate logic further down is
//  identical on every platform. The POSIX versions are thin pass-throughs
//  to exactly the calls this file always made; the Windows versions give
//  the same semantics:
//    - positional read/write (ReadFile/WriteFile + OVERLAPPED offset), so
//      no seek-then-read race and no reliance on the file position;
//    - opened with FILE_SHARE_DELETE, so the file can be removed while the
//      log still has it open, as on POSIX (the test suite relies on this);
//    - binary mode, so the CRT never translates \r\n inside a frame;
//    - fsync == FlushFileBuffers, a real durability barrier.
// ============================================================
namespace {

#if defined(_WIN32)

using FileOffset = std::int64_t;
using IoResult = std::int64_t;

// Largest single ReadFile/WriteFile request; larger buffers are returned
// as a short count and the caller's existing loop / short-read handling
// deals with it, exactly as it would for a POSIX short read.
constexpr std::size_t MAX_IO_BYTES = std::size_t{1} << 30;

HANDLE handleOf(int fd) noexcept {
    return reinterpret_cast<HANDLE>(::_get_osfhandle(fd));
}

OVERLAPPED overlappedAt(FileOffset offset) noexcept {
    OVERLAPPED ov{};
    const auto u = static_cast<std::uint64_t>(offset);
    ov.Offset = static_cast<DWORD>(u & 0xFFFFFFFFull);
    ov.OffsetHigh = static_cast<DWORD>(u >> 32);
    return ov;
}

int sysOpen(const char* path) noexcept {
    const HANDLE h =
        ::CreateFileW(std::filesystem::path(path).c_str(), GENERIC_READ | GENERIC_WRITE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return -1;

    const int fd = ::_open_osfhandle(reinterpret_cast<intptr_t>(h), _O_RDWR | _O_BINARY);
    if (fd < 0)
        ::CloseHandle(h); // the CRT didn't take ownership of the handle
    return fd;
}

int sysClose(int fd) noexcept {
    return ::_close(fd); // also closes the underlying OS handle
}

FileOffset sysFileSize(int fd) noexcept {
    LARGE_INTEGER size{};
    if (!::GetFileSizeEx(handleOf(fd), &size))
        return -1;
    return static_cast<FileOffset>(size.QuadPart);
}

IoResult sysPread(int fd, void* buf, std::size_t count, FileOffset offset) noexcept {
    OVERLAPPED ov = overlappedAt(offset);
    const auto want = static_cast<DWORD>(count < MAX_IO_BYTES ? count : MAX_IO_BYTES);
    DWORD got = 0;
    if (!::ReadFile(handleOf(fd), buf, want, &got, &ov)) {
        return ::GetLastError() == ERROR_HANDLE_EOF ? 0 : -1; // reading at/after EOF: 0 bytes
    }
    return static_cast<IoResult>(got);
}

IoResult sysPwrite(int fd, const void* buf, std::size_t count, FileOffset offset) noexcept {
    OVERLAPPED ov = overlappedAt(offset);
    const auto want = static_cast<DWORD>(count < MAX_IO_BYTES ? count : MAX_IO_BYTES);
    DWORD put = 0;
    if (!::WriteFile(handleOf(fd), buf, want, &put, &ov))
        return -1;
    return static_cast<IoResult>(put);
}

int sysFsync(int fd) noexcept {
    return ::FlushFileBuffers(handleOf(fd)) ? 0 : -1;
}

int sysFtruncate(int fd, FileOffset size) noexcept {
    return ::_chsize_s(fd, size) == 0 ? 0 : -1; // _chsize_s returns an errno code, not -1
}

#else // POSIX

using FileOffset = off_t;
using IoResult = ssize_t;

int sysOpen(const char* path) noexcept {
    return ::open(path, O_RDWR | O_CREAT, 0644);
}
int sysClose(int fd) noexcept {
    return ::close(fd);
}
FileOffset sysFileSize(int fd) noexcept {
    return ::lseek(fd, 0, SEEK_END);
}
IoResult sysPread(int fd, void* buf, std::size_t count, FileOffset offset) noexcept {
    return ::pread(fd, buf, count, offset);
}
IoResult sysPwrite(int fd, const void* buf, std::size_t count, FileOffset offset) noexcept {
    return ::pwrite(fd, buf, count, offset);
}
int sysFsync(int fd) noexcept {
    return ::fsync(fd);
}
int sysFtruncate(int fd, FileOffset size) noexcept {
    return ::ftruncate(fd, size);
}

#endif

} // namespace

// -----------------------------------------------------------------------
// Design note
//
// Frame layout on disk (see WriteAheadLog.h for the full field table):
//   [frameLength][index][term][payloadLength][payload][checksum]
// frameLength covers every field after itself, so open()'s recovery scan
// can read just the 4-byte prefix, know exactly how many more bytes the
// record should occupy, and tell "not enough bytes left" (crash mid-
// append) apart from "enough bytes but checksum mismatch" (corruption) --
// both cases stop the scan and truncate the file at the last known-good
// record, rather than treating either as a hard error.
// -----------------------------------------------------------------------

namespace {
// Header field sizes, named so the frame layout above and the read/write
// offsets below can't silently drift apart.
constexpr std::size_t FRAME_LENGTH_SIZE = sizeof(std::uint32_t);
constexpr std::size_t INDEX_SIZE = sizeof(LogIndex);
constexpr std::size_t TERM_SIZE = sizeof(Term);
constexpr std::size_t PAYLOAD_LENGTH_SIZE = sizeof(std::uint32_t);
constexpr std::size_t CHECKSUM_SIZE = sizeof(std::uint32_t);

// Bytes covered by frameLength: index + term + payloadLength + payload +
// checksum, i.e. everything after the frameLength field itself.
constexpr std::uint32_t bodySize(std::uint32_t payloadLength) noexcept {
    return static_cast<std::uint32_t>(INDEX_SIZE + TERM_SIZE + PAYLOAD_LENGTH_SIZE) +
           payloadLength + static_cast<std::uint32_t>(CHECKSUM_SIZE);
}
} // namespace

// ============================================================
//  Section 1 — Checksum
// ============================================================
std::uint32_t WriteAheadLog::crc32(const void* data, std::size_t size) noexcept {
    static const std::array<std::uint32_t, 256> table = [] {
        std::array<std::uint32_t, 256> t{};
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            t[i] = c;
        }
        return t;
    }();

    const auto* bytes = static_cast<const unsigned char*>(data);
    std::uint32_t crc = 0xFFFFFFFFu;
    for (std::size_t i = 0; i < size; ++i) {
        crc = table[(crc ^ bytes[i]) & 0xFFu] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

// ============================================================
//  Section 2 — Constructors & Destructor
// ============================================================
WriteAheadLog::WriteAheadLog(std::string path) : path_(std::move(path)) {}

WriteAheadLog::~WriteAheadLog() {
    if (fd_ >= 0)
        sysClose(fd_);
}

WriteAheadLog::WriteAheadLog(WriteAheadLog&& other) noexcept
    : path_(std::move(other.path_)), fd_(other.fd_), offsetIndex_(std::move(other.offsetIndex_)),
      lastIndex_(other.lastIndex_), lastTerm_(other.lastTerm_), writeOffset_(other.writeOffset_) {
    other.fd_ = -1;
    other.lastIndex_ = INVALID_LOG_INDEX;
    other.lastTerm_ = INVALID_TERM;
    other.writeOffset_ = 0;
    other.offsetIndex_.clear();
}

WriteAheadLog& WriteAheadLog::operator=(WriteAheadLog&& other) noexcept {
    if (this != &other) {
        if (fd_ >= 0)
            sysClose(fd_);

        path_ = std::move(other.path_);
        fd_ = other.fd_;
        offsetIndex_ = std::move(other.offsetIndex_);
        lastIndex_ = other.lastIndex_;
        lastTerm_ = other.lastTerm_;
        writeOffset_ = other.writeOffset_;

        other.fd_ = -1;
        other.lastIndex_ = INVALID_LOG_INDEX;
        other.lastTerm_ = INVALID_TERM;
        other.writeOffset_ = 0;
        other.offsetIndex_.clear();
    }

    return *this;
}

// ============================================================
//  Section 3 — Lifecycle
// ============================================================
Status WriteAheadLog::open() {
    if (fd_ >= 0) {
        sysClose(fd_);
        fd_ = -1;
    }

    fd_ = sysOpen(path_.c_str());
    if (fd_ < 0)
        return Status::IO_ERROR;

    offsetIndex_.clear();
    lastIndex_ = INVALID_LOG_INDEX;
    lastTerm_ = INVALID_TERM;
    writeOffset_ = 0;

    const FileOffset fileSize = sysFileSize(fd_);
    if (fileSize < 0) {
        sysClose(fd_);
        fd_ = -1;
        return Status::IO_ERROR;
    }

    std::uint64_t pos = 0;
    const auto totalSize = static_cast<std::uint64_t>(fileSize);

    while (pos + FRAME_LENGTH_SIZE <= totalSize) {
        std::uint32_t frameLength = 0;
        const IoResult lenRead =
            sysPread(fd_, &frameLength, FRAME_LENGTH_SIZE, static_cast<FileOffset>(pos));
        if (lenRead < 0) {
            sysClose(fd_);
            fd_ = -1;
            return Status::IO_ERROR;
        }
        if (static_cast<std::size_t>(lenRead) < FRAME_LENGTH_SIZE) {
            break; // short read of the length prefix itself: crash mid-write, stop here
        }

        if (pos + FRAME_LENGTH_SIZE + frameLength > totalSize) {
            break; // frameLength claims more bytes than the file has: crash mid-append, stop here
        }

        std::vector<char> body(frameLength);
        const IoResult bodyRead = sysPread(fd_, body.data(), frameLength,
                                           static_cast<FileOffset>(pos + FRAME_LENGTH_SIZE));
        if (bodyRead < 0) {
            sysClose(fd_);
            fd_ = -1;
            return Status::IO_ERROR;
        }
        if (static_cast<std::size_t>(bodyRead) < frameLength) {
            break; // short read of the body: crash mid-append, stop here
        }

        std::uint32_t payloadLength = 0;
        std::memcpy(&payloadLength, body.data() + INDEX_SIZE + TERM_SIZE, PAYLOAD_LENGTH_SIZE);

        const bool consistent =
            frameLength == bodySize(payloadLength) &&
            frameLength >= INDEX_SIZE + TERM_SIZE + PAYLOAD_LENGTH_SIZE + CHECKSUM_SIZE;
        if (!consistent) {
            break; // internally inconsistent frame: corrupt, stop here
        }

        std::uint32_t storedChecksum = 0;
        std::memcpy(&storedChecksum, body.data() + (frameLength - CHECKSUM_SIZE), CHECKSUM_SIZE);

        std::vector<char> checksumInput(FRAME_LENGTH_SIZE + frameLength - CHECKSUM_SIZE);
        std::memcpy(checksumInput.data(), &frameLength, FRAME_LENGTH_SIZE);
        std::memcpy(checksumInput.data() + FRAME_LENGTH_SIZE, body.data(),
                    frameLength - CHECKSUM_SIZE);

        if (crc32(checksumInput.data(), checksumInput.size()) != storedChecksum) {
            break; // checksum mismatch: corrupt tail, stop here
        }

        LogIndex entryIndex = 0;
        Term entryTerm = 0;
        std::memcpy(&entryIndex, body.data(), INDEX_SIZE);
        std::memcpy(&entryTerm, body.data() + INDEX_SIZE, TERM_SIZE);

        offsetIndex_.push_back(pos);
        lastIndex_ = entryIndex;
        lastTerm_ = entryTerm;
        pos += FRAME_LENGTH_SIZE + frameLength;
    }

    // pos now sits exactly at the end of the last valid, fully-durable
    // record. Anything beyond it (a partial or corrupt trailing record) is
    // discarded from the file itself, not just skipped in memory -- otherwise
    // a later append() would leave that garbage stranded between two valid
    // records instead of overwriting it.
    if (pos != totalSize && sysFtruncate(fd_, static_cast<FileOffset>(pos)) != 0) {
        sysClose(fd_);
        fd_ = -1;
        return Status::IO_ERROR;
    }

    writeOffset_ = pos;
    return Status::OK;
}

// ============================================================
//  Section 4 — Append
// ============================================================
Status WriteAheadLog::append(Term term, const std::string& payload, LogIndex& outIndex) {
    if (fd_ < 0)
        return Status::IO_ERROR;

    const auto payloadLength = static_cast<std::uint32_t>(payload.size());
    const std::uint32_t frameLen = bodySize(payloadLength);
    const LogIndex newIndex = lastIndex_ + 1;

    std::vector<char> buf(FRAME_LENGTH_SIZE + frameLen);
    std::size_t off = 0;
    std::memcpy(buf.data() + off, &frameLen, FRAME_LENGTH_SIZE);
    off += FRAME_LENGTH_SIZE;
    std::memcpy(buf.data() + off, &newIndex, INDEX_SIZE);
    off += INDEX_SIZE;
    std::memcpy(buf.data() + off, &term, TERM_SIZE);
    off += TERM_SIZE;
    std::memcpy(buf.data() + off, &payloadLength, PAYLOAD_LENGTH_SIZE);
    off += PAYLOAD_LENGTH_SIZE;
    std::memcpy(buf.data() + off, payload.data(), payloadLength);
    off += payloadLength;

    const std::uint32_t checksum = crc32(buf.data(), off);
    std::memcpy(buf.data() + off, &checksum, CHECKSUM_SIZE);

    std::size_t written = 0;
    while (written < buf.size()) {
        const IoResult n = sysPwrite(fd_, buf.data() + written, buf.size() - written,
                                     static_cast<FileOffset>(writeOffset_ + written));
        if (n < 0)
            return Status::IO_ERROR; // in-memory state untouched: append() had no effect
        written += static_cast<std::size_t>(n);
    }

    if (sysFsync(fd_) != 0)
        return Status::IO_ERROR;

    offsetIndex_.push_back(writeOffset_);
    writeOffset_ += buf.size();
    lastIndex_ = newIndex;
    lastTerm_ = term;
    outIndex = newIndex;

    return Status::OK;
}

// ============================================================
//  Section 5 — Read
// ============================================================
Status WriteAheadLog::readFrameAt(std::uint64_t fileOffset, LogEntry& out) const {
    std::uint32_t frameLength = 0;
    const IoResult lenRead =
        sysPread(fd_, &frameLength, FRAME_LENGTH_SIZE, static_cast<FileOffset>(fileOffset));
    if (lenRead < 0 || static_cast<std::size_t>(lenRead) < FRAME_LENGTH_SIZE) {
        return Status::IO_ERROR; // fileOffset only ever comes from offsetIndex_: a short read here
    } // means the on-disk file shrank out from under us, not corruption.

    std::vector<char> body(frameLength);
    const IoResult bodyRead = sysPread(fd_, body.data(), frameLength,
                                       static_cast<FileOffset>(fileOffset + FRAME_LENGTH_SIZE));
    if (bodyRead < 0 || static_cast<std::size_t>(bodyRead) < frameLength) {
        return Status::IO_ERROR;
    }

    std::uint32_t payloadLength = 0;
    std::memcpy(&payloadLength, body.data() + INDEX_SIZE + TERM_SIZE, PAYLOAD_LENGTH_SIZE);

    std::memcpy(&out.index, body.data(), INDEX_SIZE);
    std::memcpy(&out.term, body.data() + INDEX_SIZE, TERM_SIZE);
    out.payload.assign(body.data() + INDEX_SIZE + TERM_SIZE + PAYLOAD_LENGTH_SIZE, payloadLength);

    return Status::OK;
}

Status WriteAheadLog::entryAt(LogIndex index, LogEntry& out) const {
    if (index == INVALID_LOG_INDEX || index > lastIndex_)
        return Status::NOT_FOUND;
    return readFrameAt(offsetIndex_[index - 1], out);
}

Status WriteAheadLog::range(LogIndex fromIndex, LogIndex toIndex, Vector<LogEntry>& out) const {
    out.clear();

    if (fromIndex == INVALID_LOG_INDEX || fromIndex > toIndex || toIndex > lastIndex_) {
        return Status::NOT_FOUND;
    }

    for (LogIndex i = fromIndex; i <= toIndex; ++i) {
        LogEntry entry;
        if (Status s = readFrameAt(offsetIndex_[i - 1], entry); s != Status::OK) {
            out.clear();
            return s;
        }
        out.push_back(std::move(entry));
    }

    return Status::OK;
}

// ============================================================
//  Section 6 — Truncate
// ============================================================
Status WriteAheadLog::truncateFrom(LogIndex index) {
    if (fd_ < 0)
        return Status::IO_ERROR;

    if (index == INVALID_LOG_INDEX) {
        if (sysFtruncate(fd_, 0) != 0)
            return Status::IO_ERROR;
        offsetIndex_.clear();
        lastIndex_ = INVALID_LOG_INDEX;
        lastTerm_ = INVALID_TERM;
        writeOffset_ = 0;
        return Status::OK;
    }

    if (index > lastIndex_)
        return Status::OK; // nothing at or after index: already satisfied

    const LogIndex newLastIndex = index - 1;
    const std::uint64_t cutOffset = offsetIndex_[index - 1];
    Term newLastTerm = INVALID_TERM;

    if (newLastIndex != INVALID_LOG_INDEX) {
        // Read the new tail's term BEFORE truncating: if this fails, nothing
        // has been mutated yet, so file and in-memory state stay consistent
        // with each other. Truncating first and reading after would risk
        // leaving writeOffset_/offsetIndex_ describing a log longer than
        // what's actually on disk if this read then failed.
        LogEntry tailEntry;
        if (Status s = readFrameAt(offsetIndex_[newLastIndex - 1], tailEntry); s != Status::OK) {
            return s;
        }
        newLastTerm = tailEntry.term;
    }

    if (sysFtruncate(fd_, static_cast<FileOffset>(cutOffset)) != 0)
        return Status::IO_ERROR;

    // Rebuilt via push_back into a fresh Vector rather than resize/pop_back/
    // erase: Database.cpp's use of Vector::remove_if is the only evidence
    // I have that VectorPro::Vector isn't a drop-in std::vector, so this
    // sticks to methods actually seen in use elsewhere in the codebase.
    Vector<std::uint64_t> kept;
    for (LogIndex i = 0; i < newLastIndex; ++i) {
        kept.push_back(offsetIndex_[i]);
    }
    offsetIndex_ = std::move(kept);

    lastIndex_ = newLastIndex;
    lastTerm_ = newLastTerm;
    writeOffset_ = cutOffset;

    return Status::OK;
}

// ============================================================
//  Section 7 — Introspection
// ============================================================
LogIndex WriteAheadLog::lastIndex() const noexcept {
    return lastIndex_;
}
Term WriteAheadLog::lastTerm() const noexcept {
    return lastTerm_;
}

} // namespace MiniDB::Storage
