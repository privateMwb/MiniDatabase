# ============================================================
# CMakeLists.txt
# Build configuration for the MiniDB library.
# ============================================================

cmake_minimum_required(VERSION 3.20)

project(
    MiniDB
    VERSION 1.0.1
    LANGUAGES CXX
)

# ============================================================================
# C++ Configuration
# ============================================================================

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ============================================================================
# Internal Dependencies
# ============================================================================

set(MINIDB_INTERNAL_LIBS
    "arenapro|ArenaAllocator|ArenaPro"
    "cachepro|LRUCache|CachePro"
    "hashmappro|HashMap|HashMapPro"
    "jsonpro|JsonParser|JsonPro"
    "poolpro|PoolAllocator|PoolPro"
    "threadpoolpro|PulseThreadPool|ThreadPoolPro"
    "vectorpro|VectorPro|VectorPro"
)

set(SUBMODULE_INCLUDE_DIRS "")
set(SUBMODULE_SOURCES "")
set(SUBMODULE_LINK_LIBS "")

foreach(entry IN LISTS MINIDB_INTERNAL_LIBS)

    string(REPLACE "|" ";" entry "${entry}")

    list(GET entry 0 port_name)
    list(GET entry 1 folder_name)
    list(GET entry 2 target_name)

    set(dep_resolved FALSE)

    # ------------------------------------------------------------------------
    # Try vcpkg/installed package using the port name.
    # ------------------------------------------------------------------------

    find_package(${port_name} CONFIG QUIET)

    if(${port_name}_FOUND)

        if(TARGET ${target_name}::${target_name})

            message(
                STATUS
                "Resolved ${port_name} -> ${target_name}::${target_name}"
            )

            list(
                APPEND
                SUBMODULE_LINK_LIBS
                ${target_name}::${target_name}
            )

            set(dep_resolved TRUE)

        endif()

    endif()

    # ------------------------------------------------------------------------
    # Try package using the project/target name.
    # ------------------------------------------------------------------------

    if(NOT dep_resolved)

        find_package(${target_name} CONFIG QUIET)

        if(${target_name}_FOUND)

            if(TARGET ${target_name}::${target_name})

                message(
                    STATUS
                    "Resolved ${target_name} -> ${target_name}::${target_name}"
                )

                list(
                    APPEND
                    SUBMODULE_LINK_LIBS
                    ${target_name}::${target_name}
                )

                set(dep_resolved TRUE)

            endif()

        endif()

    endif()

    # ------------------------------------------------------------------------
    # Local submodule fallback.
    # ------------------------------------------------------------------------

    if(NOT dep_resolved)

        set(
            repo_root
            "${CMAKE_CURRENT_SOURCE_DIR}/libs/internal/${folder_name}"
        )

        set(
            include_dir
            "${repo_root}/include"
        )

        if(EXISTS "${include_dir}")

            message(
                STATUS
                "Using local submodule: ${repo_root}"
            )

            list(
                APPEND
                SUBMODULE_INCLUDE_DIRS
                "${include_dir}"
            )

            file(
                GLOB_RECURSE
                repo_sources
                CONFIGURE_DEPENDS
                "${repo_root}/src/*.cpp"
            )

            if(repo_sources)

                message(
                    STATUS
                    "Compiling submodule sources from: ${repo_root}/src"
                )

                list(
                    APPEND
                    SUBMODULE_SOURCES
                    ${repo_sources}
                )

            endif()

        else()

            message(
                FATAL_ERROR
                "Could not resolve internal dependency '${port_name}'. "
                "No installed package or local submodule was found at "
                "'${repo_root}'."
            )

        endif()

    endif()

endforeach()

# ============================================================================
# External Dependencies
# ============================================================================

file(
    GLOB
    EXTERNAL_LIB_INCLUDE_DIRS
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/external/*/include"
)

list(
    APPEND
    SUBMODULE_INCLUDE_DIRS
    ${EXTERNAL_LIB_INCLUDE_DIRS}
)

foreach(include_dir IN LISTS EXTERNAL_LIB_INCLUDE_DIRS)

    get_filename_component(
        repo_root
        "${include_dir}"
        DIRECTORY
    )

    file(
        GLOB_RECURSE
        repo_sources
        CONFIGURE_DEPENDS
        "${repo_root}/src/*.cpp"
    )

    if(repo_sources)

        message(
            STATUS
            "Compiling external submodule sources from: ${repo_root}/src"
        )

        list(
            APPEND
            SUBMODULE_SOURCES
            ${repo_sources}
        )

    endif()

endforeach()

# ============================================================================
# MiniDB Sources
# ============================================================================

file(
    GLOB_RECURSE
    LIB_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/${PROJECT_NAME}/*.cpp"
)

list(
    APPEND
    LIB_SOURCES
    ${SUBMODULE_SOURCES}
)

# ============================================================================
# MiniDB Library
# ============================================================================

if(LIB_SOURCES)

    add_library(
        ${PROJECT_NAME}
        STATIC
        ${LIB_SOURCES}
    )

    target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    target_compile_features(
        ${PROJECT_NAME}
        PUBLIC
            cxx_std_23
    )

    set(MINIDB_VISIBILITY PUBLIC)

else()

    add_library(
        ${PROJECT_NAME}
        INTERFACE
    )

    target_include_directories(
        ${PROJECT_NAME}
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    target_compile_features(
        ${PROJECT_NAME}
        INTERFACE
            cxx_std_23
    )

    set(MINIDB_VISIBILITY INTERFACE)

endif()

# ============================================================================
# vcpkg Include Directory
# ============================================================================
#
# vcpkg installs dependency headers under:
#
#   <VCPKG_INSTALLED_DIR>/<VCPKG_TARGET_TRIPLET>/include
#
# For example:
#
#   /tmp/vcpkg/installed/x64-linux/include/JsonPro/Json.h
#
# This is explicitly added because MiniDB public headers directly include
# dependency headers such as:
#
#   #include <JsonPro/Json.h>
#
# ============================================================================

if(
    DEFINED VCPKG_INSTALLED_DIR
    AND DEFINED VCPKG_TARGET_TRIPLET
)

    set(
        MINIDB_VCPKG_INCLUDE_DIR
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include"
    )

    if(EXISTS "${MINIDB_VCPKG_INCLUDE_DIR}")

        message(
            STATUS
            "vcpkg include directory: ${MINIDB_VCPKG_INCLUDE_DIR}"
        )

        target_include_directories(
            ${PROJECT_NAME}
            ${MINIDB_VISIBILITY}
                $<BUILD_INTERFACE:${MINIDB_VCPKG_INCLUDE_DIR}>
        )

    endif()

endif()

# ============================================================================
# Submodule Include Directories
# ============================================================================

foreach(dir IN LISTS SUBMODULE_INCLUDE_DIRS)

    message(
        STATUS
        "Include: ${dir}"
    )

    target_include_directories(
        ${PROJECT_NAME}
        ${MINIDB_VISIBILITY}
            $<BUILD_INTERFACE:${dir}>
            $<INSTALL_INTERFACE:include>
    )

endforeach()

# ============================================================================
# Link Internal Package Targets
# ============================================================================

if(SUBMODULE_LINK_LIBS)

    target_link_libraries(
        ${PROJECT_NAME}
        ${MINIDB_VISIBILITY}
            ${SUBMODULE_LINK_LIBS}
    )

endif()

# ============================================================================
# Subdirectories
# ============================================================================

option(
    BUILD_TESTS
    "Build the test suite"
    ON
)

option(
    BUILD_BENCHMARKS
    "Build the benchmark suite"
    ON
)

option(
    BUILD_REGRESSION
    "Build the regression benchmark tools"
    ON
)

option(
    BUILD_EXAMPLES
    "Build the examples suite"
    ON
)

enable_testing()

if(BUILD_TESTS)

    add_subdirectory(
        tests
        tests_build
    )

endif()

if(BUILD_BENCHMARKS)

    add_subdirectory(
        benchmarks
        benchmarks_build
    )

endif()

if(BUILD_REGRESSION)

    add_subdirectory(
        regression
        regression_build
    )

endif()

if(BUILD_EXAMPLES)

    add_subdirectory(
        examples
        examples_build
    )

endif()

# ============================================================================
# Installation
# ============================================================================

install(
    TARGETS
        ${PROJECT_NAME}
    EXPORT
        ${PROJECT_NAME}Targets
)

install(
    EXPORT
        ${PROJECT_NAME}Targets
    FILE
        ${PROJECT_NAME}Targets.cmake
    NAMESPACE
        ${PROJECT_NAME}::
    DESTINATION
        lib/cmake/${PROJECT_NAME}
)

install(
    DIRECTORY
        include/
    DESTINATION
        include
)

# Install local submodule headers when building from source.
foreach(dir IN LISTS SUBMODULE_INCLUDE_DIRS)

    if(EXISTS "${dir}")

        install(
            DIRECTORY
                "${dir}/"
            DESTINATION
                include
        )

    endif()

endforeach()

# ============================================================================
# Packaging
# ============================================================================

include(CMakePackageConfigHelpers)

configure_package_config_file(
    cmake/${PROJECT_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION
        lib/cmake/${PROJECT_NAME}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION
        ${PROJECT_VERSION}
    COMPATIBILITY
        SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION
        lib/cmake/${PROJECT_NAME}
)