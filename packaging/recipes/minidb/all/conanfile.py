from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy
from conan.tools.scm import Git
import os


class Conan(ConanFile):
    # ── Retargeting this recipe for a new library ───────────────────
    # Edit these fields (and the class name above) -- everything below
    # derives from them. Version is handled by a separate script, not
    # edited here.
    name = "minidb"
    cmake_name = "MiniDB"  # matches project()'s name in the top-level CMakeLists.txt
    version = "1.0.0"

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

    def validate(self):
        check_min_cppstd(self, 23)

    def source(self):
        # NOTE: not using conan.tools.files.get() with a GitHub tarball URL
        # here. Like vcpkg_from_github, that downloads a plain archive with
        # no .git directory and no submodule content -- MiniDB's own
        # internal libraries (ArenaAllocator, LRUCache, HashMap, JsonParser,
        # PoolAllocator, PulseThreadPool, VectorPro) live under
        # libs/internal/ as real git submodules, which need an actual
        # clone + submodule checkout to materialize (a bare `get()` would
        # silently leave those directories empty, failing later at compile
        # time with a much more confusing "header not found" error instead
        # of here).
        #
        # `git clone --branch` only accepts branches/tags, not arbitrary
        # commit SHAs, so pinning to a specific commit needs the
        # init/fetch/checkout sequence below instead. Once checked out,
        # `submodule update` reads .gitmodules from the real clone and
        # resolves each submodule's own pinned commit automatically --
        # no per-submodule hash bookkeeping needed here, unlike the vcpkg
        # port (which has to fetch submodule archives manually since
        # GitHub tarballs never include them).
        #
        # Trade-off: this loses the sha256 archive-integrity pin get()
        # would otherwise give -- acceptable for a private recipe against
        # our own repo, not a package intended for conan-center.
        sources = self.conan_data["sources"][self.version]
        git = Git(self)
        git.run("init .")
        git.run(f"remote add origin {sources['url']}")
        git.run(f"fetch --depth 1 origin {sources['commit']}")
        git.run("checkout FETCH_HEAD")
        git.run("submodule sync --recursive")
        git.run("submodule update --init --recursive")

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
