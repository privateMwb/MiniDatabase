# ============================================================
# .clusterfuzzlite/Dockerfile
# Base image for building the fuzz harnesses under ClusterFuzzLite.
# ============================================================

FROM gcr.io/oss-fuzz-base/base-builder@sha256:5f1a3ea9278fb85c79de740fe6fbcbb4948fbb81e71cc1c6f21e0abc86681e8c

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
