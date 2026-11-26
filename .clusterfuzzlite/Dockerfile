# ============================================================
# .clusterfuzzlite/Dockerfile
# Base image for building the fuzz harnesses under ClusterFuzzLite.
# ============================================================

FROM gcr.io/oss-fuzz-base/base-builder@sha256:3f007cb3f3f0e3f9d0b7f51fa0bbec822c310bcf786023205c6385703ad60032

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
