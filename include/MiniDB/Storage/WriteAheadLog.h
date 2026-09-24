/**
 * @file            WriteAheadLog.h
 *
 * @date            2026-9-2
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
#include <string>  // std::string (path, payload)
#include <cstdint> // std::uint32_t, std::uint64_t
// clang-format on

#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>  // ::open, O_RDWR/O_CREAT
#include <unistd.h> // ::pread, ::pwrite, ::close, ::fsync, ::ftruncate, ::lseek
#endif

#include <MiniDB/Common/Type.h>

// clang-format off
#include <VectorPro/Vector.h> // VectorPro::Vector (per-entry offset index)
// clang-format on

// An append-only, crash-recoverable log of (index, term, payload) entries,
// independent of Database's whole-file JSON snapshot path and
// StorageEngine's fixed-slot page path (see Common/FileIO.h) -- entries
// are variable-length and framed individually, not fixed-size slots,
// since a WAL is written and scanned sequentially rather than accessed by
// random slot offset. Reuses FileIO's raw-POSIX-fd style (pread/pwrite/
// fsync) rather than std::fstream, for the same true-random-access and
// fsync-backed durability reasons FileIO itself documents. POSIX-only for
// now (no std::fstream fallback like FileIO has) -- acceptable given the
// current dev target; revisit if a non-POSIX build ever matters. Not
// Raft-specific: general enough for any "durable append log" consumer
// (event sourcing, audit logs, transaction logs); Raft's replicated log
// is simply the first concrete consumer.
//
// On-disk record framing (all fields native-endian, matching FileIO's own
// slot format -- not portable across differing-endianness hosts, an
// acceptable tradeoff for a single-host embedded engine, same call
// writeSlot() already makes):
//
//   [uint32 frameLength]   length of every field below, in bytes
//   [uint64 index]         monotonic log index, assigned by append()
//   [uint64 term]          caller-supplied term/version tag
//   [uint32 payloadLength] length of payload, in bytes
//   [byte[] payload]       opaque entry data
//   [uint32 checksum]      CRC32 over [frameLength .. payload], i.e. every
//                          field above including frameLength itself
//
// frameLength lets recovery scanning (see open()) read one 4-byte prefix,
// then know exactly how many more bytes the record should occupy --
// distinguishing "not enough bytes left in the file" (crash mid-append,
// discard and stop) from "enough bytes but checksum mismatch" (corruption,
// also discard and stop) without guessing. No shared checksum utility
// exists elsewhere in MiniDB yet, so CRC32 is implemented privately here
// rather than introduced as a new cross-module dependency for one caller.

namespace MiniDB::Storage {

using namespace MiniDB::Common;
using namespace VectorPro;

using LogIndex = std::uint64_t; ///< 1-based monotonic index of a log entry.
using Term = std::uint64_t;     ///< Caller-supplied term/version tag, opaque to the log itself.

/// @brief Sentinel for "no entries appended yet".
constexpr LogIndex INVALID_LOG_INDEX = 0;
/// @brief Sentinel for "no term recorded yet".
constexpr Term INVALID_TERM = 0;

/// @brief One durable log entry as returned by read operations (Phase 2).
struct LogEntry {
    LogIndex index = INVALID_LOG_INDEX;
    Term term = INVALID_TERM;
    std::string payload;
};

/**
 * @brief An append-only, crash-recoverable log of (index, term, payload)
 * entries backed by a single file.
 * @details See the file-level design note for the on-disk record format
 * and rationale. `open()` performs a full sequential scan to rebuild the
 * in-memory offset index and `lastIndex_`/`lastTerm_` -- necessary even
 * for a clean-shutdown reopen, since there is no separate metadata file
 * recording where the log left off. A trailing record that fails its
 * checksum, or is shorter than its own `frameLength` (a crash mid-
 * `append`), is truncated from the file during this same scan rather than
 * treated as a hard error; this folds the roadmap's Phase 4 recovery
 * mechanism into `open()` itself, since Phase 1 cannot correctly resume
 * appending to a pre-existing file without it.
 */
class WriteAheadLog {
  private:
    std::string path_; ///< Path of the backing log file.
    int fd_ = -1;      ///< Open file descriptor; -1 when closed.

    /// Byte offset of each entry's frame (the `frameLength` field),
    /// indexed by `LogIndex - 1`. Rebuilt by `open()`, extended by
    /// `append()`. Populated now so Phase 2's `entryAt()`/`range()` can be
    /// added as pure readers of this member, with no further scanning.
    Vector<std::uint64_t> offsetIndex_;

    LogIndex lastIndex_ = INVALID_LOG_INDEX; ///< Index of the last durable entry.
    Term lastTerm_ = INVALID_TERM;           ///< Term of the last durable entry.
    std::uint64_t writeOffset_ = 0;          ///< Byte offset the next append() writes at.

    /// @brief Computes the CRC32 (IEEE 802.3 polynomial) checksum of
    /// `size` bytes starting at `data`.
    [[nodiscard]] static std::uint32_t crc32(const void* data, std::size_t size) noexcept;

    /// @brief Reads and parses the frame at byte offset `fileOffset` into
    /// `out`. Shared by `entryAt()` and `range()`; does not re-verify the
    /// checksum (`open()` already did, and `fileOffset` is only ever a
    /// value this class itself put into `offsetIndex_`).
    /// @return `Status::OK` on success, `Status::IO_ERROR` on a read
    /// failure.
    [[nodiscard]] Status readFrameAt(std::uint64_t fileOffset, LogEntry& out) const;

  public:
    /// @brief Constructs a log bound to `path`. Performs no I/O; call
    /// `open()` before use.
    /// @param path Path of the backing log file. Created by `open()` if
    /// it doesn't already exist.
    explicit WriteAheadLog(std::string path);

    /// @brief Closes the backing file descriptor, if open.
    ~WriteAheadLog();

    WriteAheadLog(const WriteAheadLog&) = delete;
    WriteAheadLog& operator=(const WriteAheadLog&) = delete;

    /**
     * @brief Move-constructs a log, taking ownership of `other`'s open
     * file descriptor and in-memory state.
     * @param other Log to move from. Left closed (`fd_ == -1`) and empty.
     */
    WriteAheadLog(WriteAheadLog&& other) noexcept;

    /**
     * @brief Move-assigns from `other`, closing this log's own file
     * descriptor first.
     * @param other Log to move from. Left closed (`fd_ == -1`) and empty.
     * @return Reference to `*this`.
     */
    WriteAheadLog& operator=(WriteAheadLog&& other) noexcept;

    /**
     * @brief Opens (creating if missing) the backing file and rebuilds
     * in-memory state by scanning every entry.
     * @return `Status::OK` on success; `Status::IO_ERROR` if the file
     * can't be opened/created, or a read fails outright (as opposed to
     * simply running out of bytes, which is the ordinary crash-recovery
     * case -- see class-level details).
     */
    [[nodiscard]] Status open();

    /**
     * @brief Durably appends one entry.
     * @param term Caller-supplied term/version tag, stored verbatim.
     * @param payload Opaque entry data.
     * @param outIndex Set to the newly assigned index on success.
     * @return `Status::OK` on success; `Status::IO_ERROR` if the log
     * isn't open, or on write/`fsync` failure. On failure, the log's
     * in-memory state is left exactly as it was before the call -- a
     * failed append never advances `lastIndex()`.
     * @details Every call `fsync`s before returning. The roadmap's
     * fsync-batching question is deferred, not decided against --
     * every-write is the safe default until reasoned through against the
     * Raft paper's actual durability requirement.
     */
    [[nodiscard]] Status append(Term term, const std::string& payload, LogIndex& outIndex);

    /**
     * @brief Reads the entry at `index`.
     * @param index 1-based log index to read.
     * @param out Overwritten with the entry on success.
     * @return `Status::OK` on success; `Status::NOT_FOUND` if `index` is
     * `INVALID_LOG_INDEX` or greater than `lastIndex()`; `Status::IO_ERROR`
     * on a read failure.
     */
    [[nodiscard]] Status entryAt(LogIndex index, LogEntry& out) const;

    /**
     * @brief Reads every entry in `[fromIndex, toIndex]` -- inclusive on
     * both ends, matching what a Raft leader would ask for when catching a
     * follower up.
     * @param fromIndex First index to read, inclusive.
     * @param toIndex Last index to read, inclusive.
     * @param out Cleared, then filled with entries in ascending index
     * order.
     * @return `Status::OK` on success; `Status::NOT_FOUND` if the range is
     * empty (`fromIndex > toIndex`), starts before `1`, or ends past
     * `lastIndex()`; `Status::IO_ERROR` on a read failure. `out` is
     * cleared even on failure, never partially filled.
     */
    [[nodiscard]] Status range(LogIndex fromIndex, LogIndex toIndex, Vector<LogEntry>& out) const;

    /**
     * @brief Discards every entry from `index` onward, keeping
     * `[1, index-1]`.
     * @param index First index to discard. `INVALID_LOG_INDEX` (`0`)
     * discards everything.
     * @return `Status::OK` on success -- including when `index >
     * lastIndex()`, a no-op since there's nothing at or after `index` to
     * begin with; `Status::IO_ERROR` if the log isn't open, the term of
     * the new tail can't be read, or the underlying truncate fails. On
     * failure the log (both on disk and in memory) is left exactly as it
     * was before the call.
     */
    [[nodiscard]] Status truncateFrom(LogIndex index);

    /// @brief Returns the index of the last durable entry, or
    /// `INVALID_LOG_INDEX` if the log is empty.
    [[nodiscard]] LogIndex lastIndex() const noexcept;
    /// @brief Returns the term of the last durable entry, or
    /// `INVALID_TERM` if the log is empty.
    [[nodiscard]] Term lastTerm() const noexcept;
};

} // namespace MiniDB::Storage
