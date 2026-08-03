# ============================================================
# CMakeLists.txt
# Build configuration for the MiniDB library.
# ============================================================
#
# Sections:
#   1. C++ Configuration
#   2. Dependencies
#   3. MiniDB Library
#   4. Subdirectories
#   5. Installation
#   6. Packaging
#
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

# Place all executables directly in the build directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ============================================================================
# Dependencies
# ============================================================================
#
# MiniDB depends on:
#
#   ArenaPro
#   CachePro
#   HashMapPro
#   JsonPro
#   PoolPro
#   ThreadPoolPro
#   VectorPro
#
# Packaged builds:
#   Dependencies are resolved through find_package().
#
# Local development:
#   If a package is not installed, the corresponding local submodule is used.
#
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

    string(REPLACE "|" ";" entry_parts "${entry}")

    list(GET entry_parts 0 port_name)
    list(GET entry_parts 1 folder_name)
    list(GET entry_parts 2 target_name)

    set(dep_resolved FALSE)

    # ------------------------------------------------------------------------
    # Try package name first.
    # ------------------------------------------------------------------------

    find_package(${port_name} CONFIG QUIET)

    if(${port_name}_FOUND)

        if(TARGET ${target_name}::${target_name})

            message(
                STATUS
                "MiniDB: resolved ${port_name} -> ${target_name}::${target_name}"
            )

            list(
                APPEND
                SUBMODULE_LINK_LIBS
                ${target_name}::${target_name}
            )

            set(dep_resolved TRUE)

        elseif(TARGET ${target_name})

            message(
                STATUS
                "MiniDB: resolved ${port_name} -> ${target_name}"
            )

            list(
                APPEND
                SUBMODULE_LINK_LIBS
                ${target_name}
            )

            set(dep_resolved TRUE)

        else()

            message(
                WARNING
                "MiniDB: package '${port_name}' was found, but target "
                "'${target_name}::${target_name}' was not exported."
            )

        endif()

    endif()

    # ------------------------------------------------------------------------
    # Try project/target name as fallback.
    # ------------------------------------------------------------------------

    if(NOT dep_resolved)

        find_package(${target_name} CONFIG QUIET)

        if(${target_name}_FOUND)

            if(TARGET ${target_name}::${target_name})

                message(
                    STATUS
                    "MiniDB: resolved ${port_name} -> ${target_name}::${target_name}"
                )

                list(
                    APPEND
                    SUBMODULE_LINK_LIBS
                    ${target_name}::${target_name}
                )

                set(dep_resolved TRUE)

            elseif(TARGET ${target_name})

                message(
                    STATUS
                    "MiniDB: resolved ${port_name} -> ${target_name}"
                )

                list(
                    APPEND
                    SUBMODULE_LINK_LIBS
                    ${target_name}
                )

                set(dep_resolved TRUE)

            endif()

        endif()

    endif()

    # ------------------------------------------------------------------------
    # Fall back to local submodule.
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
                "MiniDB: ${port_name} package not found; "
                "using local submodule at ${repo_root}"
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
                    "MiniDB: compiling submodule sources from "
                    "${repo_root}/src"
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
                "Could not resolve internal dependency '${port_name}'.\n"
                "No installed package was found and the local submodule "
                "does not exist at:\n"
                "  ${repo_root}\n\n"
                "Run:\n"
                "  git submodule update --init --recursive\n"
                "or make sure the dependency is available through "
                "vcpkg/Conan."
            )

        endif()

    endif()

endforeach()

# ============================================================================
# External submodules
# ============================================================================
#
# Third-party libraries under libs/external/ remain local submodules.
#
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
            "MiniDB: compiling external sources from ${repo_root}/src"
        )

        list(
            APPEND
            SUBMODULE_SOURCES
            ${repo_sources}
        )

    endif()

endforeach()

# ============================================================================
# MiniDB Library
# ============================================================================
#
# If MiniDB has .cpp files:
#     STATIC library
#
# If MiniDB has no .cpp files:
#     INTERFACE header-only library
#
# ============================================================================

file(
    GLOB_RECURSE
    LIB_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/${PROJECT_NAME}/*.cpp"
)

# Add locally compiled submodule sources.
list(
    APPEND
    LIB_SOURCES
    ${SUBMODULE_SOURCES}
)

if(LIB_SOURCES)

    # ------------------------------------------------------------------------
    # Compiled MiniDB library.
    # ------------------------------------------------------------------------

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

    set(
        MINIDB_VISIBILITY
        PUBLIC
    )

    # ------------------------------------------------------------------------
    # Link packaged dependencies.
    #
    # This is important for vcpkg:
    #
    # JsonPro::JsonPro
    # VectorPro::VectorPro
    # etc.
    #
    # Their exported targets carry their own include directories.
    # ------------------------------------------------------------------------

    if(SUBMODULE_LINK_LIBS)

        target_link_libraries(
            ${PROJECT_NAME}
            PUBLIC
                ${SUBMODULE_LINK_LIBS}
        )

    endif()

else()

    # ------------------------------------------------------------------------
    # Header-only MiniDB library.
    # ------------------------------------------------------------------------

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

    set(
        MINIDB_VISIBILITY
        INTERFACE
    )

    if(SUBMODULE_LINK_LIBS)

        target_link_libraries(
            ${PROJECT_NAME}
            INTERFACE
                ${SUBMODULE_LINK_LIBS}
        )

    endif()

endif()

# ============================================================================
# Local submodule include directories
# ============================================================================

foreach(dir IN LISTS SUBMODULE_INCLUDE_DIRS)

    target_include_directories(
        ${PROJECT_NAME}
        ${MINIDB_VISIBILITY}
        $<BUILD_INTERFACE:${dir}>
    )

endforeach()

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

# Install MiniDB public headers.
install(
    DIRECTORY
        include/
    DESTINATION
        include
)

# Install locally resolved submodule headers.
#
# These are needed only when the dependencies were built from local
# submodules rather than installed packages.
#
# Packaged dependencies such as JsonPro are normally installed separately
# by vcpkg.
foreach(dir IN LISTS SUBMODULE_INCLUDE_DIRS)

    install(
        DIRECTORY
            ${dir}/
        DESTINATION
            include
    )

endforeach()

# ============================================================================
# Packaging
# ============================================================================

include(
    CMakePackageConfigHelpers
)

# Generate MiniDBConfig.cmake.
configure_package_config_file(
    cmake/${PROJECT_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION
        lib/cmake/${PROJECT_NAME}
)

# Generate MiniDBConfigVersion.cmake.
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION
        ${PROJECT_VERSION}
    COMPATIBILITY
        SameMajorVersion
)

# Install package configuration files.
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION
        lib/cmake/${PROJECT_NAME}
)