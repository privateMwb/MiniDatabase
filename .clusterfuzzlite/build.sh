#!/bin/bash -eu
# ============================================================
# .clusterfuzzlite/build.sh
#
# Each harness below compiles against a curated subset of
# MiniDatabase's sources, not the whole library.
#
# Layout notes (verified against the repo, not assumed):
#   - FileIO is header-only (include/MiniDB/Common/FileIO.h); there is
#     no src/MiniDB/Common/ directory at all.
#   - JSON comes from the JsonParser submodule (JsonPro::Json). Unlike
#     VectorPro it is NOT header-only, so its src/JsonPro/*.cpp must be
#     compiled into any harness that touches Record.
#   - <MiniDB/MiniDatabase.h> pulls in every module, so every
#     libs/internal/*/include directory has to be on the include path,
#     even for a harness that only links one or two .cpp files.
#
# Add more `fuzz/fuzz_*.cpp` harnesses here as they're added; each
# becomes its own $OUT binary. If a new harness needs a source file
# not already listed for it below, add it to that harness's own compile
# line -- don't widen an existing harness's sources.
# ============================================================

cd "${SRC}/MiniDatabase"

# Fail with a clear message, not a wall of "file not found" compiler
# errors, if the Dockerfile's submodule step didn't populate libs/.
if [ ! -d libs/internal/JsonParser/src/JsonPro ]; then
  echo "error: libs/internal submodules are not checked out" >&2
  exit 1
fi

INCLUDES="-I${SRC}/MiniDatabase/include"
for dir in "${SRC}"/MiniDatabase/libs/internal/*/include; do
  INCLUDES="${INCLUDES} -I${dir}"
done

JSONPRO_SOURCES="$(ls libs/internal/JsonParser/src/JsonPro/*.cpp)"

# fuzz_record_deserialize: Record::deserialize() -- the entry point for
# every byte MiniDatabase ever reads back out of a saved Table/Page's
# JSON, before any of Record::validate()'s own schema checking has run.
# Needs Record.cpp plus JsonPro's implementation (Json::parse()).
$CXX $CXXFLAGS -std=c++23 $INCLUDES \
  fuzz/fuzz_record_deserialize.cpp \
  src/MiniDB/Core/Record.cpp \
  ${JSONPRO_SOURCES} \
  $LIB_FUZZING_ENGINE \
  -o "${OUT}/fuzz_record_deserialize"

# fuzz_wal_recovery: WriteAheadLog::open()'s crash-recovery scan over a
# fuzzer-supplied on-disk log file -- a genuinely different untrusted-
# bytes surface than the JSON path above (a binary, CRC32-framed
# format read directly off disk, not through Json::parse at all).
# WriteAheadLog.cpp is self-contained (its I/O goes straight to the OS).
$CXX $CXXFLAGS -std=c++23 $INCLUDES \
  fuzz/fuzz_wal_recovery.cpp \
  src/MiniDB/Storage/WriteAheadLog.cpp \
  $LIB_FUZZING_ENGINE \
  -o "${OUT}/fuzz_wal_recovery"
