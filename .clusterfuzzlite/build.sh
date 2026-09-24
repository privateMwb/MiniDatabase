#!/bin/bash -eu
# ============================================================
# .clusterfuzzlite/build.sh
#
# Each harness below compiles against a curated subset of
# MiniDatabase's sources, not the whole library -- mirroring
# FalconHTTP's build.sh convention in this ecosystem.
#
# ASSUMPTION: the include/source paths below follow MiniDatabase's
# documented layout (include/MiniDB/<SubNamespace>/*.h,
# src/MiniDB/<SubNamespace>/*.cpp) and the `rain::` ecosystem's usual
# libs/internal/<Repo>/include submodule pattern for VectorPro
# (Vector<>) and ArenaAllocator (Arena<>, ARENA_SIZE). Neither harness
# below has been compiled against the real repo -- verify these paths
# and adjust before relying on this script.
#
# Add more `${SRC}/MiniDatabase/fuzz/fuzz_*.cpp` harnesses here as
# they're added; each becomes its own $OUT binary. If a new harness
# needs a source file/submodule not already listed for it below, add
# it to that harness's own compile line -- don't widen an existing
# harness's sources just to make a new one easier to bolt on.
# ============================================================

cd "${SRC}/MiniDatabase"

INCLUDES="-I${SRC}/MiniDatabase/include \
  -I${SRC}/MiniDatabase/libs/internal/VectorPro/include \
  -I${SRC}/MiniDatabase/libs/internal/ArenaAllocator/include"

# fuzz_record_deserialize: Record::deserialize() -- the entry point for
# every byte MiniDatabase ever reads back out of a saved Table/Page's
# JSON, before any of Record::validate()'s own schema checking has run.
# Needs Record.cpp plus whatever Common source implements Json::parse().
$CXX $CXXFLAGS -std=c++20 $INCLUDES \
  fuzz/fuzz_record_deserialize.cpp \
  src/MiniDB/Core/Record.cpp \
  src/MiniDB/Common/Json.cpp \
  $LIB_FUZZING_ENGINE \
  -o "${OUT}/fuzz_record_deserialize"

# fuzz_wal_recovery: WriteAheadLog::open()'s crash-recovery scan over a
# fuzzer-supplied on-disk log file -- a genuinely different untrusted-
# bytes surface than the JSON path above (a binary, CRC32-framed
# format read directly off disk, not through Json::parse at all).
# Needs WriteAheadLog.cpp plus FileIO.cpp for its slot-level I/O.
$CXX $CXXFLAGS -std=c++20 $INCLUDES \
  fuzz/fuzz_wal_recovery.cpp \
  src/MiniDB/Storage/WriteAheadLog.cpp \
  src/MiniDB/Common/FileIO.cpp \
  $LIB_FUZZING_ENGINE \
  -o "${OUT}/fuzz_wal_recovery"
