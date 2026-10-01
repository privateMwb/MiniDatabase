# ============================================================
# .clusterfuzzlite/Dockerfile
# Base image for building the fuzz harnesses under ClusterFuzzLite.
# ============================================================

FROM gcr.io/oss-fuzz-base/base-builder@sha256:e67da2b0f6156b29b62734305a8c74db61aaf3158c7ab96386a07974fb33cdc8

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
