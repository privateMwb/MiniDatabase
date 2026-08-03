# ============================================================
# CMakeLists.txt
# Build configuration for the MiniDB library.
# ============================================================
#
# Sections:
#   1. C++ Configuration
#   2. External / Internal Submodule Libraries
#   3. MiniDB Library
#   4. Subdirectories
#   5. Installation
#   6. Packaging
#
# ============================================================

cmake_minimum_required(VERSION 3.20)

project(
    MiniDB
    VERSION 1.0.0
    LANGUAGES CXX
)

# ============================================================================
# C++ Configuration
# ============================================================================

# Configure the C++ language standard.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Place all executables directly in the build directory, regardless of
# which subdirectory defines them (e.g. build/tests, build/benchmarks).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ============================================================================
# External / Internal Submodule Libraries
# ============================================================================
#
# lib/internal/  -> libraries owned/maintained by this project (ArenaPro,
#                    VectorPro, JsonPro, MiniDB, StackPro, FunctionPro...)
# lib/external/  -> third-party libraries pinned as submodules
#
# Both follow the same layout convention: <repo>/include/<LibName>/*.h
# so we only need to glob each repo's include/ directory, not the files
# themselves. This keeps #include paths short, e.g. <JsonPro/Json.h>
# regardless of how deep the submodule sits in lib/.
#
# Deliberately NOT using add_subdirectory() + target_link_libraries() here:
# that requires every submodule's own CMakeLists.txt to define an
# add_library() target whose name we'd have to guess correctly (folder
# names and target/namespace names don't always match -- e.g. JsonParser/
# vs JsonPro).
#
# IMPORTANT: not every submodule is actually header-only (e.g. JsonPro,
# PulseThreadPool ship compiled .cpp files). Gluing in include/ dirs alone
# gets you declarations without definitions -> undefined symbol errors at
# link time. So alongside each include/ dir, we also glob the matching
# src/ dir (same <repo>/src/<LibName>/*.cpp convention) and compile those
# sources directly into MiniDB itself. Truly header-only submodules simply
# contribute an empty src/ glob and cost nothing extra.

file(GLOB INTERNAL_LIB_INCLUDE_DIRS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/internal/*/include"
)

file(GLOB EXTERNAL_LIB_INCLUDE_DIRS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/external/*/include"
)

set(SUBMODULE_INCLUDE_DIRS
    ${INTERNAL_LIB_INCLUDE_DIRS}
    ${EXTERNAL_LIB_INCLUDE_DIRS}
)

foreach(dir ${SUBMODULE_INCLUDE_DIRS})
    message(STATUS "Include: ${dir}")
endforeach()

# Compile each submodule's own sources into MiniDB directly, so libraries
# that aren't header-only (JsonPro, PulseThreadPool, ...) actually get
# linked instead of just declared.
set(SUBMODULE_SOURCES "")
foreach(include_dir ${SUBMODULE_INCLUDE_DIRS})
    get_filename_component(repo_root "${include_dir}" DIRECTORY)
    file(GLOB_RECURSE repo_sources CONFIGURE_DEPENDS
        "${repo_root}/src/*.cpp"
    )
    if(repo_sources)
        message(STATUS "Compiling submodule sources from: ${repo_root}/src")
        list(APPEND SUBMODULE_SOURCES ${repo_sources})
    endif()
endforeach()

# ============================================================================
# MiniDB Library (auto: header-only INTERFACE if no .cpp files, STATIC otherwise)
# ============================================================================

# Collect any library source files under src/MiniDB/.
# CONFIGURE_DEPENDS makes CMake re-run this glob on reconfigure, so
# adding sources to a previously header-only project is picked up
# automatically on the next cmake run -- no manual editing required.
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/${PROJECT_NAME}/*.cpp"
)

# Fold in any submodule sources so a submodule that isn't header-only
# (e.g. JsonPro, PulseThreadPool) still gets compiled and linked, and so
# MiniDB correctly becomes a STATIC target rather than INTERFACE the
# moment any of them has real .cpp files, even if MiniDB itself doesn't.
list(APPEND LIB_SOURCES ${SUBMODULE_SOURCES})

if(LIB_SOURCES)
    # Sources found (Database.cpp, Table.cpp, Page.cpp, Record.cpp,
    # QueryEngine.cpp, Concurrency.cpp, Serializer.cpp, StorageEngine.cpp,
    # etc., plus any compiled submodule sources): build a real compiled
    # static library.
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
    # No sources: header-only library, INTERFACE target.
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

# Expose each submodule include directory individually.
# NOTE: Do NOT interpolate SUBMODULE_INCLUDE_DIRS directly inside a
# generator expression (e.g. $<BUILD_INTERFACE:${SUBMODULE_INCLUDE_DIRS}>).
# It's a semicolon-separated list, and CMake's command parser splits on
# semicolons before the genex is evaluated, corrupting the first entry.
# Looping and wrapping one path per call keeps each genex well-formed.
#
# MINIDB_VISIBILITY matches whichever branch above actually ran: PUBLIC
# for the compiled STATIC case, INTERFACE for the header-only case (an
# INTERFACE target cannot use PUBLIC/PRIVATE).
foreach(dir ${SUBMODULE_INCLUDE_DIRS})
    target_include_directories(
        ${PROJECT_NAME}
        ${MINIDB_VISIBILITY}
        $<BUILD_INTERFACE:${dir}>
        $<INSTALL_INTERFACE:include>
    )
endforeach()

# ============================================================================
# Subdirectories
# ============================================================================

# tests/, benchmarks/, and examples/ each own their own CMakeLists.txt
# and define their own executable target(s) from their own source layout
# (e.g. tests/suite/concurrency/*.cpp, tests/suite/integration/*.cpp, ...).
# Consumers (e.g. vcpkg) should disable these -- only the library itself
# needs to be configured/installed for downstream projects. Defaults to
# ON so local development and existing CI workflows are unaffected.
option(BUILD_TESTS "Build the test suite" ON)
option(BUILD_BENCHMARKS "Build the benchmark suite" ON)
option(BUILD_REGRESSION "Build the regression benchmark tools" ON)
option(BUILD_EXAMPLES "Build the examples suite" ON)

enable_testing()

if(BUILD_TESTS)
    add_subdirectory(tests tests_build)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks benchmarks_build)
endif()

if(BUILD_REGRESSION)
    add_subdirectory(regression regression_build)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples examples_build)
endif()

# ============================================================================
# Installation
# ============================================================================

# Install the MiniDB target and export it for downstream projects.
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
)

# Install the exported CMake targets.
install(
    EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake/${PROJECT_NAME}
)

# Install the public header files.
install(
    DIRECTORY include/
    DESTINATION include
)

# Install each submodule's public headers alongside MiniDB's own, since
# MiniDB's public headers (e.g. Record.h) include them directly (e.g.
# <JsonPro/Json.h>). Every submodule follows the same
# <repo>/include/<LibName>/*.h layout, so flattening each into the shared
# include/ destination reproduces that layout without any renaming.
foreach(dir ${SUBMODULE_INCLUDE_DIRS})
    install(
        DIRECTORY ${dir}/
        DESTINATION include
    )
endforeach()

# ============================================================================
# Packaging
# ============================================================================

include(CMakePackageConfigHelpers)

# Generate the package configuration file.
configure_package_config_file(
    cmake/${PROJECT_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
)

# Generate the package version file.
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the package configuration files.
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)
