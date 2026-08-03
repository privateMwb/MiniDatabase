vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO privateMwb/MiniDatabase
    REF f4c51cfc09cfe8b6824b5e9c43ca4ea92b0e6387
    SHA512 3c329f1e73acf8fa0cbfc745b1b4e03e78c1cf10ec4c8987e8e300290fb258f22b6fcb3cd11a79f3a751c286b5402ef54055154ade982dfa1508b24314edc791
)

# GitHub archive tarballs never include submodule content, so MiniDB's
# internal libraries under libs/internal/ are fetched separately here,
# each pinned to the exact commit the submodule points at, then copied
# into place.
vcpkg_from_github(
    OUT_SOURCE_PATH ARENAALLOCATOR_SOURCE_PATH
    REPO privateMwb/ArenaAllocator
    REF 7fe76fb4c3f1d98edeb34832834baece8481de79
    SHA512 39eceb4db5d18d3a5339da7a761772f956dc13521726605bd3683ad0ce62a2c55cf7ac9c546ce626badfc571a83b2193ed86fcae12020e7d168381a592de5f92
)
vcpkg_from_github(
    OUT_SOURCE_PATH HASHMAP_SOURCE_PATH
    REPO privateMwb/HashMap
    REF 497b996dfa4721136a4c89edefd36e6a0dfe1d09
    SHA512 3d696060c666b9ea9afa02e9394f637967851cf167d864ec2699858b02370fca52ad8c796552be726d0534373fa828aec0879aaa9ba17d9692b902c199c8eba9
)
vcpkg_from_github(
    OUT_SOURCE_PATH JSONPARSER_SOURCE_PATH
    REPO privateMwb/JsonParser
    REF e4668f451d9bfd14d09f55bb4613e40be893e30e
    SHA512 421980c1f8f5b327a8b8d61ba0bb9eb860081ee713daa7a9c866de260a19752e8affba22cffd28c689f791c4ce53650000acfe9efe187e976a22026b009859a4
)
vcpkg_from_github(
    OUT_SOURCE_PATH LRUCACHE_SOURCE_PATH
    REPO privateMwb/LRUCache
    REF b82b2f00aaafdc205693760ec0e0e191752b95b6
    SHA512 c8d606e2ee9814b6abdfeae00d12ac935e04a06aa5bd79efd04ebb065e54a06e0df95f84ca00e19624d91a4d95c70154ad0a0b0b9caa5713fed513eb7574dfde
)
vcpkg_from_github(
    OUT_SOURCE_PATH POOLALLOCATOR_SOURCE_PATH
    REPO privateMwb/PoolAllocator
    REF 13c8f82dfb6ed910b5464c068a346d7857e26d0d
    SHA512 9760b0da16aee0c1eea7e3b457bc5140b1ca336652df7ec931fa274d161a4db1d1ffe739f0fc4bcd7edbf66d64af692eb1d0b188abc19c7903e609a988fff96b
)
vcpkg_from_github(
    OUT_SOURCE_PATH PULSETHREADPOOL_SOURCE_PATH
    REPO privateMwb/PulseThreadPool
    REF 260864a7ec3cb2c5135cd2effe402b20974c04f0
    SHA512 6d6314f614aa29184079d89727eefa730a9298452329a047a412c71ede467f15398d06d4409e8776dc7b76eae384cfaf16b8e0a0f220afc9f6e4424c4150d285
)
vcpkg_from_github(
    OUT_SOURCE_PATH VECTORPRO_SOURCE_PATH
    REPO privateMwb/VectorPro
    REF 26407a59ecd6fefe69565c980b8de49332e469e8
    SHA512 969c97bfad58f94a75ba35c8723812badad112d5b304e73b555715ad61657055a7c41c6568fadeb5e8a21bca27215435abef28e6a1221ff14f6d2244ab3ca081
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