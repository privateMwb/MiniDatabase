# ============================================================
# .clusterfuzzlite/Dockerfile
# Base image for building the fuzz harnesses under ClusterFuzzLite.
# ============================================================

FROM gcr.io/oss-fuzz-base/base-builder@sha256:14b5fea57004f2f4edcfa212aa8c77397244f307a37eb216277ac40ec0155b1e

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
