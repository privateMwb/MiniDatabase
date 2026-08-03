vcpkg_find_acquire_program(GIT)

# NOTE: not using vcpkg_from_github here. It downloads a GitHub archive
# tarball, which -- unlike a real git clone -- never contains submodule
# content and has no .git directory, so a follow-up
# `git submodule update --init` has nothing to operate on and fails
# outright (confirmed against other vcpkg ports hitting the same issue,
# e.g. https://github.com/microsoft/vcpkg/issues/27004). MiniDB's own
# internal libraries (ArenaPro, CachePro, HashMapPro, JsonPro, PoolPro,
# ThreadPoolPro, VectorPro) live under libs/internal/ as real git
# submodules and need an actual clone to materialize.
#
# Trade-off: this loses vcpkg_from_github's SHA512 archive-integrity
# pinning (a plain `git clone` has no equivalent built-in check) --
# acceptable here since this is a private overlay port for our own repo,
# not a port intended for the public vcpkg registry.
set(SOURCE_PATH "${CURRENT_BUILDTREES_DIR}/src/minidb-v1.0.0")
file(REMOVE_RECURSE "${SOURCE_PATH}")

vcpkg_execute_required_process(
    COMMAND "${GIT}" clone
        --branch v1.0.0
        --depth 1
        --recurse-submodules
        --shallow-submodules
        https://github.com/privateMwb/MiniDatabase.git
        "${SOURCE_PATH}"
    WORKING_DIRECTORY "${CURRENT_BUILDTREES_DIR}/src"
    LOGNAME "minidb-clone-${TARGET_TRIPLET}"
)

set(VCPKG_PORT_NAME MiniDB)

# Consumers only need the library itself, not the tests, benchmarks,
# regression tools, or examples.
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_TESTS=OFF
        -DBUILD_BENCHMARKS=OFF
        -DBUILD_REGRESSION=OFF
        -DBUILD_EXAMPLES=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(
    PACKAGE_NAME ${VCPKG_PORT_NAME}
    CONFIG_PATH lib/cmake/${VCPKG_PORT_NAME}
)

# This library is compiled (not header-only), so debug binaries are
# real and must be kept — only the duplicate debug/include headers
# are removed.
file(
    REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
)

vcpkg_install_copyright(
    FILE_LIST "${SOURCE_PATH}/LICENSE"
)