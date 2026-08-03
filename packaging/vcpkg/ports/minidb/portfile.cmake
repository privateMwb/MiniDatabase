vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO privateMwb/MiniDatabase
    REF v1.0.1
    SHA512 1bbb75c4c498f0d018d9143c5326a6029e44a839ed61821dc6e1cc5ec9f5c5c7d76bf4825aea9d3d035c2b4cb8d33b34673ed5ed1db8e76f88ae1f86242eba1f
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