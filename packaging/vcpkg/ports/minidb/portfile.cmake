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
    OUT_SOURCE_PATH ARENAALLOCATOR_SOURCE_PATH
    REPO privateMwb/ArenaAllocator
    REF REPLACE_WITH_ARENAALLOCATOR_COMMIT_SHA
    SHA512 REPLACE_WITH_ARENAALLOCATOR_SHA512
)
vcpkg_from_github(
    OUT_SOURCE_PATH HASHMAP_SOURCE_PATH
    REPO privateMwb/HashMap
    REF REPLACE_WITH_HASHMAP_COMMIT_SHA
    SHA512 REPLACE_WITH_HASHMAP_SHA512
)
vcpkg_from_github(
    OUT_SOURCE_PATH JSONPARSER_SOURCE_PATH
    REPO privateMwb/JsonParser
    REF REPLACE_WITH_JSONPARSER_COMMIT_SHA
    SHA512 REPLACE_WITH_JSONPARSER_SHA512
)
vcpkg_from_github(
    OUT_SOURCE_PATH LRUCACHE_SOURCE_PATH
    REPO privateMwb/LRUCache
    REF REPLACE_WITH_LRUCACHE_COMMIT_SHA
    SHA512 REPLACE_WITH_LRUCACHE_SHA512
)
vcpkg_from_github(
    OUT_SOURCE_PATH POOLALLOCATOR_SOURCE_PATH
    REPO privateMwb/PoolAllocator
    REF REPLACE_WITH_POOLALLOCATOR_COMMIT_SHA
    SHA512 REPLACE_WITH_POOLALLOCATOR_SHA512
)
vcpkg_from_github(
    OUT_SOURCE_PATH PULSETHREADPOOL_SOURCE_PATH
    REPO privateMwb/PulseThreadPool
    REF REPLACE_WITH_PULSETHREADPOOL_COMMIT_SHA
    SHA512 REPLACE_WITH_PULSETHREADPOOL_SHA512
)
vcpkg_from_github(
    OUT_SOURCE_PATH VECTORPRO_SOURCE_PATH
    REPO privateMwb/VectorPro
    REF REPLACE_WITH_VECTORPRO_COMMIT_SHA
    SHA512 REPLACE_WITH_VECTORPRO_SHA512
)

foreach(SUBMODULE_NAME ArenaAllocator HashMap JsonParser LRUCache PoolAllocator PulseThreadPool VectorPro)
    file(REMOVE_RECURSE "${SOURCE_PATH}/libs/internal/${SUBMODULE_NAME}")
endforeach()
file(RENAME "${ARENAALLOCATOR_SOURCE_PATH}" "${SOURCE_PATH}/libs/internal/ArenaAllocator")
file(RENAME "${HASHMAP_SOURCE_PATH}" "${SOURCE_PATH}/libs/internal/HashMap")
file(RENAME "${JSONPARSER_SOURCE_PATH}" "${SOURCE_PATH}/libs/internal/JsonParser")
file(RENAME "${LRUCACHE_SOURCE_PATH}" "${SOURCE_PATH}/libs/internal/LRUCache")
file(RENAME "${POOLALLOCATOR_SOURCE_PATH}" "${SOURCE_PATH}/libs/internal/PoolAllocator")
file(RENAME "${PULSETHREADPOOL_SOURCE_PATH}" "${SOURCE_PATH}/libs/internal/PulseThreadPool")
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