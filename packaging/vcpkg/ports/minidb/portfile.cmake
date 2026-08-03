vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO privateMwb/MiniDatabase
    REF REPLACE_WITH_COMMIT_SHA
    SHA512 REPLACE_WITH_SHA512
)

# GitHub archive tarballs never include submodule content, so MiniDB's
# internal libraries under libs/internal/ are fetched separately here,
# each pinned to the exact commit the submodule points at, then copied
# into place.
vcpkg_from_github(
    OUT_SOURCE_PATH ARENAPRO_SOURCE_PATH
    REPO privateMwb/ArenaPro
    REF REPLACE_WITH_ARENAPRO_COMMIT_SHA
    SHA512 REPLACE_WITH_ARENAPRO_SHA512
)
vcpkg_from_github(
    OUT_SOURCE_PATH CACHEPRO_SOURCE_PATH
    REPO privateMwb/CachePro
    REF REPLACE_WITH_CACHEPRO_COMMIT_SHA
    SHA512 REPLACE_WITH_CACHEPRO_SHA512
)
vcpkg_from_github(
    OUT_SOURCE_PATH HASHMAPPRO_SOURCE_PATH
    REPO privateMwb/HashMapPro
    REF REPLACE_WITH_HASHMAPPRO_COMMIT_SHA
    SHA512 REPLACE_WITH_HASHMAPPRO_SHA512
)
vcpkg_from_github(
    OUT_SOURCE_PATH JSONPRO_SOURCE_PATH
    REPO privateMwb/JsonPro
    REF REPLACE_WITH_JSONPRO_COMMIT_SHA
    SHA512 REPLACE_WITH_JSONPRO_SHA512
)
vcpkg_from_github(
    OUT_SOURCE_PATH POOLPRO_SOURCE_PATH
    REPO privateMwb/PoolPro
    REF REPLACE_WITH_POOLPRO_COMMIT_SHA
    SHA512 REPLACE_WITH_POOLPRO_SHA512
)
vcpkg_from_github(
    OUT_SOURCE_PATH THREADPOOLPRO_SOURCE_PATH
    REPO privateMwb/ThreadPoolPro
    REF REPLACE_WITH_THREADPOOLPRO_COMMIT_SHA
    SHA512 REPLACE_WITH_THREADPOOLPRO_SHA512
)
vcpkg_from_github(
    OUT_SOURCE_PATH VECTORPRO_SOURCE_PATH
    REPO privateMwb/VectorPro
    REF REPLACE_WITH_VECTORPRO_COMMIT_SHA
    SHA512 REPLACE_WITH_VECTORPRO_SHA512
)

foreach(SUBMODULE_NAME ARENAPRO CACHEPRO HASHMAPPRO JSONPRO POOLPRO THREADPOOLPRO VECTORPRO)
    file(REMOVE_RECURSE "${SOURCE_PATH}/libs/internal/${SUBMODULE_NAME}")
endforeach()
file(RENAME "${ARENAPRO_SOURCE_PATH}" "${SOURCE_PATH}/libs/internal/ArenaPro")
file(RENAME "${CACHEPRO_SOURCE_PATH}" "${SOURCE_PATH}/libs/internal/CachePro")
file(RENAME "${HASHMAPPRO_SOURCE_PATH}" "${SOURCE_PATH}/libs/internal/HashMapPro")
file(RENAME "${JSONPRO_SOURCE_PATH}" "${SOURCE_PATH}/libs/internal/JsonPro")
file(RENAME "${POOLPRO_SOURCE_PATH}" "${SOURCE_PATH}/libs/internal/PoolPro")
file(RENAME "${THREADPOOLPRO_SOURCE_PATH}" "${SOURCE_PATH}/libs/internal/ThreadPoolPro")
file(RENAME "${VECTORPRO_SOURCE_PATH}" "${SOURCE_PATH}/libs/internal/VectorPro")

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
