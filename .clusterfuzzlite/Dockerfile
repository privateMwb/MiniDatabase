# ============================================================
# .clusterfuzzlite/Dockerfile
# Base image for building the fuzz harnesses under ClusterFuzzLite.
# ============================================================

FROM gcr.io/oss-fuzz-base/base-builder@sha256:8b4a73d83374b298a0a771eeec9a1d44ceff3416a678b7d7946303389d022aca

COPY . $SRC/MiniDatabase
COPY .clusterfuzzlite/build.sh $SRC/

WORKDIR $SRC/MiniDatabase

# ASSUMPTION: like FalconHTTP, MiniDatabase pulls in the ecosystem's other
# repos (e.g. VectorPro for Vector<>, ArenaAllocator for the Arena<>
# QueryEngine uses) as git submodules under libs/internal/*, per the
# `rain::` umbrella pattern shared across privateMwb projects. If
# MiniDatabase instead vendors these headers directly or has no
# submodules at all, drop this line.
RUN git submodule update --init --recursive
