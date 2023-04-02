from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout


class SkyeConan(ConanFile):
    name = "skye"
    description = "Skye is an HTTP server framework for C++20."
    homepage = "https://github.com/luketokheim/skye"
    license = "BSL-1.0"

    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv"
    exports_sources = "CMakeLists.txt", "cmake/*", "include/*"
    no_copy_source = True

    options = {
        "developer_mode": [True, False],
        "enable_arch": [True, False],
        "enable_benchmarks": [True, False],
        "enable_io_uring": [True, False]
    }
    default_options = {
        "developer_mode": False,
        "enable_arch": False,
        "enable_benchmarks": False,
        "enable_io_uring": False
    }

    def requirements(self):
        self.requires("boost/1.81.0", options={"header_only": True})
        self.requires("fmt/9.1.0")

    def build_requirements(self):
        if not self.options.developer_mode:
            return

        if self.options.enable_benchmarks:
            self.test_requires("benchmark/1.7.1")

        if not self.conf.get("tools.build:skip_test", default=False):
            self.test_requires("catch2/3.3.2")

        self.test_requires("sqlite3/3.41.1")

    def layout(self):
        cmake_layout(self)
        self.folders.generators = "conan"

    def build(self):
        variables = dict()
        if self.options.developer_mode:
            variables["skye_DEVELOPER_MODE"] = True
        if self.options.enable_arch:
            variables["ENABLE_ARCH"] = True
        if self.options.enable_benchmarks:
            variables["ENABLE_BENCHMARKS"] = True
        if self.options.enable_io_uring:
            variables["ENABLE_IO_URING"] = True

        cmake = CMake(self)
        cmake.configure(variables=variables)
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()