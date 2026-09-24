# Fuzzing

MiniDatabase is fuzzed via [ClusterFuzzLite](https://google.github.io/clusterfuzzlite/),
running on every pull request that touches a fuzzed file, plus a
longer scheduled batch run every night.

## What's covered

**`fuzz_record_deserialize.cpp`** targets `Record::deserialize()` -
the entry point for every byte MiniDatabase ever reads back out of a
Page's JSON, before `Record::validate()` has had a chance to run
against any schema. Unlike a differential fuzzer, there's no
shadow-model JSON parser to compare against, so this instead:

- treats `deserialize()`'s own documented `Status::PARSE_ERROR` (from
  malformed JSON, or JSON that doesn't match the envelope Record
  expects) as an expected, non-finding outcome for bad input
- on a successful parse, exercises every accessor -
  id/deleted-flag, `hasField`/`getField`/`getFieldRef` against a
  representative schema (both present and absent keys), `validate()`,
  and the `toJson()`/`serialize()` round trip - so ASan/UBSan can catch
  anything reachable only through a specific field combination a
  hand-written unit test wouldn't think to construct

This codebase's own regression suite
(`record_validate_zero.cpp`, under `tests/*/suite/regression/`) is a
track record of exactly the kind of malformed/unexpected-value bug
this harness exists to catch before a human has to find the next one
by hand.

**`fuzz_wal_recovery.cpp`** targets `WriteAheadLog::open()`'s
crash-recovery scan, fuzzing the on-disk log file directly - the whole
point of recovery is to survive exactly the kind of arbitrary
corruption a crash mid-write leaves behind, so raw fuzzer bytes are a
realistic proxy for "the file as some previous, interrupted process
left it," not an artificial input shape. It checks the one invariant
`open()`'s recovery contract implies: whatever `lastIndex()` open()
settles on after scanning a possibly-corrupt file, every entry from 1
through that `lastIndex()` must actually read back out cleanly via
`range()`/`entryAt()` - a recovery scan that reports a `lastIndex()` it
can't back up with real, intact entries is the specific failure mode
this harness exists to catch.

This codebase's own unit/regression coverage already has two fixed
corruption shapes - a truncated trailing record and a
checksum-mismatched trailing record, both hand-crafted at exact byte
offsets (see `write_ahead_log.cpp` under `tests/*/suite/{regression,unit}/`).
This harness is the open-ended counterpart to those two fixed cases:
arbitrary corruption, at any offset, of any shape.

Both are built and run under both AddressSanitizer and
UndefinedBehaviorSanitizer.

## What's deliberately NOT covered

- **`Page::deserialize()` / `Table::deserialize()`** - each wraps the
  same Record-level JSON parsing this harness already exercises inside
  an outer array/object. Fuzzing `Record::deserialize()` directly is
  the minimal reproduction of the shared parsing surface all three go
  through, rather than three harnesses re-fuzzing the same inner
  parser through progressively thicker wrappers.
- **`Database`'s JSON path** - `Database` has no string
  `serialize()`/`deserialize()` of its own; `save()`/`load()` go
  through `toJson()`/`fromJson()` plus `FileIO`, ultimately bottoming
  out in the same `Record`-level parsing already covered here.
- **`QueryEngine`** - operates on `FilterPredicate`/`SortCondition`
  values that are, in real usage, developer-written call arguments, not
  attacker-controlled bytes off the wire or off disk.
- **`FileIO::readSlot()`'s length-prefix parsing** - has its own fixed
  corrupt-length-prefix regression test
  (`fileIO.cpp`, `ReadSlotCorruptLengthPrefixReturnsParseError`), but
  no dedicated harness yet. A natural follow-up, structured the same
  way as `fuzz_wal_recovery.cpp`: write fuzzed bytes to a slot-sized
  file and check `readSlot()` never reads past the slot boundary.
- **`Concurrency`'s parallel batch operations** - fan out to the same
  per-table `Serializer`/`FileIO` calls already exercised (indirectly)
  by the two harnesses above; the concurrency-specific behavior worth
  checking (fold order, partial-apply-on-failure) is deterministic
  logic over `Status` values, not a parsing surface, and is already
  covered by `error_propagation.cpp` under `tests/*/suite/concurrency/`.

## Running locally

```bash
git clone --recursive https://github.com/google/oss-fuzz.git
cd oss-fuzz
python infra/helper.py build_fuzzers --sanitizer address MiniDatabase /path/to/MiniDatabase
python infra/helper.py run_fuzzer MiniDatabase fuzz_record_deserialize
python infra/helper.py run_fuzzer MiniDatabase fuzz_wal_recovery
```

Or, without OSS-Fuzz's tooling, directly with clang (submodules must
already be checked out - `git submodule update --init --recursive`):

```bash
clang++ -std=c++20 -fsanitize=fuzzer,address \
  -Iinclude -Ilibs/internal/VectorPro/include -Ilibs/internal/ArenaAllocator/include \
  fuzz/fuzz_record_deserialize.cpp \
  src/MiniDB/Core/Record.cpp \
  src/MiniDB/Common/Json.cpp \
  -o fuzz_record_deserialize

./fuzz_record_deserialize
```

```bash
clang++ -std=c++20 -fsanitize=fuzzer,address \
  -Iinclude -Ilibs/internal/VectorPro/include -Ilibs/internal/ArenaAllocator/include \
  fuzz/fuzz_wal_recovery.cpp \
  src/MiniDB/Storage/WriteAheadLog.cpp \
  src/MiniDB/Common/FileIO.cpp \
  -o fuzz_wal_recovery

./fuzz_wal_recovery
```

Add `-fsanitize=fuzzer,undefined` instead to run under UBSan.

## Reproducing a crash

ClusterFuzzLite uploads the failing input as a workflow artifact when
a run fails. Download it, then:

```bash
./fuzz_record_deserialize path/to/crash-<hash>
```

This replays that exact byte sequence through
`LLVMFuzzerTestOneInput()` once, deterministically - no sanitizer
flags needed beyond however the binary was already built.

## Adding a new harness

1. Add `fuzz/fuzz_<target>.cpp` with an `extern "C" int
   LLVMFuzzerTestOneInput(const uint8_t*, size_t)` entry point.
2. Add the matching compile + link block to
   `.clusterfuzzlite/build.sh` - list only the source files/submodules
   that harness actually needs (see the comment at the top of
   `build.sh`), not the whole library.
3. No workflow changes needed for `cflite_batch.yml` - it builds and
   runs every binary `build.sh` produces in `$OUT`. If the new harness
   targets different source files than the existing two, add them to
   `cflite_pr.yml`'s `paths:` filter so PRs touching them actually
   trigger a run.
