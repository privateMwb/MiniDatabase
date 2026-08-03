from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy, get
import os


class Conan(ConanFile):
    # ── Retargeting this recipe for a new library ───────────────────
    # Edit these fields (and the class name above) -- everything below
    # derives from them. Version is handled by a separate script, not
    # edited here.
    name = "minidb"
    cmake_name = "MiniDB"  # matches project()'s name in the top-level CMakeLists.txt
    version = "1.0.1"

    url = "https://github.com/privateMwb/MiniDatabase"
    description = "Embedded C++ database engine (in-process, no server) built on this project's own allocator, container, concurrency, and JSON libraries."
    topics = (
        "database",
        "embedded-database",
        "storage-engine",
        "cpp",
    )
    # ──────────────────────────────────────────────────────────────

    # static-library: src/MiniDB/*.cpp compiles into libMiniDB.a, so
    # consumers link a real binary rather than an INTERFACE target.
    # NOTE: if this library ever goes back to header-only, this needs to
    # flip back to "header-library" and package_id()/cpp_info.libs need to
    # become conditional again, mirroring CMakeLists.txt's own auto-detect.
    package_type = "static-library"

    license = "MIT"
    author = "privateMwb"

    settings = "os", "compiler", "build_type", "arch"

    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }

    default_options = {
        "shared": False,
        "fPIC": True,
    }

    exports_sources = (
        "CMakeLists.txt",
        "cmake/*",
        "include/*",
        "src/*",
    )

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        # All 7 are consumed directly by MiniDB's own public headers (e.g.
        # Record.h includes <VectorPro/Vector.h> and <JsonPro/Json.h> at
        # namespace scope), so both flags are needed:
        #   transitive_headers=True -- a consumer of MiniDB who includes
        #     MiniDB/Core/Record.h needs these headers on their include
        #     path too, not just MiniDB's own.
        #   transitive_libs=True -- MiniDB links these in as
        #     PUBLIC/${MINIDB_VISIBILITY} in CMakeLists.txt (see the
        #     add_subdirectory + target_link_libraries section), so a
        #     consumer linking against MiniDB needs these libs linked too.
        #
        # ASSUMPTION: package names follow the same lowercase-of-cmake_name
        # pattern this recipe itself uses (cmake_name "MiniDB" -> name
        # "minidb") -- confirmed for cachepro via packaging.yml's
        # recipes/cachepro/all path, inferred by the same pattern for the
        # other six. Verify each name/version against the actual published
        # packages before this will resolve.
        self.requires("vectorpro/1.0.0",     transitive_headers=True, transitive_libs=True)
        self.requires("jsonpro/1.0.0",       transitive_headers=True, transitive_libs=True)
        self.requires("poolpro/1.0.0",       transitive_headers=True, transitive_libs=True)
        self.requires("hashmappro/1.0.0",    transitive_headers=True, transitive_libs=True)
        self.requires("cachepro/1.0.0",      transitive_headers=True, transitive_libs=True)
        self.requires("arenapro/1.0.0",      transitive_headers=True, transitive_libs=True)
        self.requires("threadpoolpro/1.0.0", transitive_headers=True, transitive_libs=True)

    def validate(self):
        check_min_cppstd(self, 23)

    def source(self):
        get(
            self,
            **self.conan_data["sources"][self.version],
            strip_root=True,
        )

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure(
            variables={
                "BUILD_TESTS": "OFF",
                "BUILD_BENCHMARKS": "OFF",
                "BUILD_REGRESSION": "OFF",
                "BUILD_EXAMPLES": "OFF",
            }
        )
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

        copy(
            self,
            "LICENSE",
            src=self.source_folder,
            dst=os.path.join(self.package_folder, "licenses"),
        )

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", self.cmake_name)
        self.cpp_info.set_property("cmake_target_name", f"{self.cmake_name}::{self.cmake_name}")
        # Compiled static library: bindirs/libdirs must NOT be cleared
        # (that's only correct for header-only), and the actual archive
        # needs to be listed so consumers link against it.
        self.cpp_info.libs = [self.cmake_name]
