/**
 * @file            FileIO.h
 *
 * @date            2026-2-8
 *
 * @version         1.0.0
 *
 * @copyright       Copyright (c) 2026 MWB
 *                  All rights reserved.
 *
 * @attention       This source is released under the MIT license
 *                  SPDX-License-Identifier: MIT
 *                  <http://opensource.org/licenses/MIT>
 */

#pragma once

// clang-format off
#include <string>     // std::string
#include <fstream>    // std::ifstream, std::ofstream, std::fstream
#include <filesystem> // std::filesystem::file_size/rename/remove/exists
#include <cstdio>     // (platform fallback I/O)
#include <cstdint>    // std::uint32_t, std::uint64_t
#include <cstring>    // std::memcpy
#include <vector>     // std::vector (slot buffer)
// clang-format on

#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>  // ::open, O_RDONLY/O_RDWR/O_CREAT/O_TRUNC
#include <unistd.h> // ::read, ::write, ::pread, ::pwrite, ::close, ::fsync
#endif

#include <MiniDB/Common/Type.h>

// Free-function file I/O primitives used by StorageEngine and Database:
// whole-file read/write for JSON snapshots, and fixed-size random-access
// "slot" read/write for page files. POSIX builds use raw fd + pread/pwrite
// for true random access and fsync-backed durability; other platforms fall
// back to std::fstream with seek, which is portable but gives no durability
// guarantee across a power loss (see writeFileAtomic).

namespace MiniDB::Common::FileIO {

/**
 * @brief Reads the entire contents of a file into `out`.
 * @param path Path of the file to read.
 * @param out Destination string, resized and overwritten with the file's
 * contents.
 * @return `Status::OK` on success, `Status::IO_ERROR` if the file's size
 * can't be determined or it can't be opened/read.
 * @details Uses a single sized read (`file_size()` + one `resize()` +
 * one `read()`) rather than a `stringstream`/`rdbuf()` copy, avoiding
 * per-character streambuf overhead and over-allocation.
 */
[[nodiscard]] inline Status readFile(const std::string& path, std::string& out) {
    std::error_code ec;
    auto size = std::filesystem::file_size(path, ec);
    if (ec)
        return Status::IO_ERROR;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return Status::IO_ERROR;

    out.resize(static_cast<std::size_t>(size));
    if (size > 0) {
        file.read(out.data(), static_cast<std::streamsize>(size));
        if (!file)
            return Status::IO_ERROR;
    }
    return Status::OK;
}

/**
 * @brief Writes `content` to `path` atomically.
 * @param path Destination path.
 * @param content Bytes to write.
 * @return `Status::OK` on success, `Status::IO_ERROR` on any failure
 * (temp file couldn't be created/written, or the rename failed).
 * @details Writes to a sibling `path + ".tmp"` file, flushes it
 * (`fsync` on POSIX), then renames it over `path`. A crash at any point
 * before the rename leaves the original file untouched; the rename
 * itself is atomic on the same filesystem (a POSIX/NTFS guarantee, not
 * a C++ one). On non-POSIX platforms there's no portable `fsync`
 * equivalent via `std::ofstream`, so content is still written via
 * temp-file + atomic rename, but durability across a power loss (as
 * opposed to a crash of just this process) isn't guaranteed there.
 */
[[nodiscard]] inline Status writeFileAtomic(const std::string& path, const std::string& content) {
    const std::string tmp = path + ".tmp";

#if defined(__unix__) || defined(__APPLE__)
    // POSIX path: write + fsync the same fd we wrote through, so the fsync
    // actually covers the bytes we just wrote (not a stale re-opened fd).
    int fd = ::open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return Status::IO_ERROR;

    const char* data = content.data();
    std::size_t remaining = content.size();
    while (remaining > 0) {
        ssize_t n = ::write(fd, data, remaining);
        if (n < 0) {
            ::close(fd);
            return Status::IO_ERROR;
        }
        data += n;
        remaining -= static_cast<std::size_t>(n);
    }
    if (::fsync(fd) != 0) {
        ::close(fd);
        return Status::IO_ERROR;
    }
    ::close(fd);
#else
    // Fallback: no portable fsync via std::ofstream. Content is still
    // written via temp-file + atomic rename, but durability across a power
    // loss (vs. a crash of just this process) isn't guaranteed here without
    // a platform-specific flush call.
    {
        std::ofstream file(tmp, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
            return Status::IO_ERROR;
        file.write(content.data(), static_cast<std::streamsize>(content.size()));
        file.flush();
        if (!file.good())
            return Status::IO_ERROR;
    }
#endif

    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp);
        return Status::IO_ERROR;
    }
    return Status::OK;
}

/**
 * @brief Writes `payload` into fixed-size slot `slotIndex` of `path`.
 * @param path Path of the slot file. Created if it doesn't already exist.
 * @param slotIndex Zero-based slot index. Slot `slotIndex` occupies byte
 * range `[slotIndex*slotSize, (slotIndex+1)*slotSize)`.
 * @param slotSize Size of each slot, in bytes. Must be large enough to
 * hold `payload` plus a 4-byte length prefix.
 * @param payload Bytes to store in the slot.
 * @return `Status::OK` on success, `Status::OUT_OF_MEMORY` if `payload`
 * doesn't fit in a slot of `slotSize`, `Status::IO_ERROR` on any I/O
 * failure.
 * @details `path` is treated as a flat array of `slotSize`-byte slots.
 * Each slot stores a 4-byte native-endian length prefix followed by
 * `payload`, zero-padded to `slotSize`. This is the primitive that lets
 * a page file support true O(1) random-access reads/writes by page id
 * (`offset = id * PAGE_SIZE`) instead of requiring a whole-file parse.
 * A payload that doesn't fit is a hard error (`Status::OUT_OF_MEMORY`),
 * not silent truncation — callers (e.g. `StorageEngine::writePage`) must
 * keep serialized pages within budget. The length prefix uses native
 * byte order (no `htole32`/`ntohle32` conversion), so slot files are not
 * portable across machines of differing endianness — acceptable for a
 * single-host embedded engine.
 */
[[nodiscard]] inline Status writeSlot(const std::string& path, std::uint64_t slotIndex,
                                      std::uint32_t slotSize, const std::string& payload) {
    if (payload.size() + sizeof(std::uint32_t) > slotSize) {
        // Payload does not fit in a fixed slot of this size. Caller (e.g.
        // StorageEngine::writePage) must keep serialized pages within
        // budget -- this is intentionally a hard error, not silent
        // truncation/corruption.
        return Status::OUT_OF_MEMORY;
    }

    std::vector<char> buf(slotSize, 0);
    std::uint32_t len = static_cast<std::uint32_t>(payload.size());
    std::memcpy(buf.data(), &len, sizeof(len));
    std::memcpy(buf.data() + sizeof(len), payload.data(), payload.size());

#if defined(__unix__) || defined(__APPLE__)
    int fd = ::open(path.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd < 0)
        return Status::IO_ERROR;

    off_t offset = static_cast<off_t>(slotIndex) * static_cast<off_t>(slotSize);
    std::size_t written = 0;
    while (written < buf.size()) {
        ssize_t n = ::pwrite(fd, buf.data() + written, buf.size() - written,
                             offset + static_cast<off_t>(written));
        if (n < 0) {
            ::close(fd);
            return Status::IO_ERROR;
        }
        written += static_cast<std::size_t>(n);
    }
    ::close(fd);
#else
    // Fallback: create the file first if it doesn't exist (fstream can't
    // open a nonexistent file with in|out), then seek + write in place.
    if (!std::filesystem::exists(path)) {
        std::ofstream create(path, std::ios::binary);
        if (!create.is_open())
            return Status::IO_ERROR;
    }
    std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open())
        return Status::IO_ERROR;

    file.seekp(static_cast<std::streamoff>(slotIndex) * static_cast<std::streamoff>(slotSize));
    file.write(buf.data(), static_cast<std::streamsize>(buf.size()));
    if (!file.good())
        return Status::IO_ERROR;
#endif

    return Status::OK;
}

/**
 * @brief Reads fixed-size slot `slotIndex` from `path` (see `writeSlot`).
 * @param path Path of the slot file.
 * @param slotIndex Zero-based slot index to read.
 * @param slotSize Size of each slot, in bytes. Must match the value
 * `writeSlot` was called with.
 * @param outPayload Destination string, overwritten with the slot's
 * stored payload on success.
 * @return `Status::OK` on success; `Status::NOT_FOUND` if the file
 * doesn't exist, the slot is beyond current end-of-file, or the slot was
 * never written (zero length prefix) — callers should treat this as "no
 * such page yet", not a hard error; `Status::IO_ERROR` on a read
 * failure; `Status::PARSE_ERROR` if the stored length prefix is larger
 * than the slot itself (corrupt data).
 */
[[nodiscard]] inline Status readSlot(const std::string& path, std::uint64_t slotIndex,
                                     std::uint32_t slotSize, std::string& outPayload) {
    std::vector<char> buf(slotSize, 0);

#if defined(__unix__) || defined(__APPLE__)
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0)
        return Status::NOT_FOUND;

    off_t offset = static_cast<off_t>(slotIndex) * static_cast<off_t>(slotSize);
    std::size_t total = 0;
    while (total < buf.size()) {
        ssize_t n =
            ::pread(fd, buf.data() + total, buf.size() - total, offset + static_cast<off_t>(total));
        if (n < 0) {
            ::close(fd);
            return Status::IO_ERROR;
        }
        if (n == 0)
            break; // short read: slot partially/never written (EOF)
        total += static_cast<std::size_t>(n);
    }
    ::close(fd);
    if (total < sizeof(std::uint32_t))
        return Status::NOT_FOUND;
#else
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return Status::NOT_FOUND;

    file.seekg(static_cast<std::streamoff>(slotIndex) * static_cast<std::streamoff>(slotSize));
    file.read(buf.data(), static_cast<std::streamsize>(buf.size()));
    if (file.gcount() < static_cast<std::streamsize>(sizeof(std::uint32_t))) {
        return Status::NOT_FOUND;
    }
#endif

    std::uint32_t len = 0;
    std::memcpy(&len, buf.data(), sizeof(len));
    if (len == 0)
        return Status::NOT_FOUND; // never written
    if (static_cast<std::uint64_t>(len) + sizeof(len) > slotSize) {
        return Status::PARSE_ERROR; // corrupt length prefix
    }

    outPayload.assign(buf.data() + sizeof(len), len);
    return Status::OK;
}

} // namespace MiniDB::Common::FileIO
