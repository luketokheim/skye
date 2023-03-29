from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout


class SkyeConan(ConanFile):
    name = "skye"
    description = "Skye is an HTTP server framework for C++20."
    homepage = "https://github.com/luketokheim/skye"
    license = "BSL-1.0"

    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeToolchain", "CMakeDeps"
    exports_sources = "CMakeLists.txt", "include/*", "examples/*", "tests/*", "bench/*"
    options = {
        "enable_arch": [True, False],
        "enable_benchmarks": [True, False],
        "enable_io_uring": [True, False]
    }
    default_options = {
        "enable_arch": False,
        "enable_benchmarks": False,
        "enable_io_uring": False
    }

    def requirements(self):
        self.test_requires("boost/1.81.0", options={"header_only": True})

        self.test_requires("fmt/9.1.0")

        self.test_requires("sqlite3/3.41.1")

        self.test_requires("catch2/3.3.2")

        if self.options.enable_benchmarks:
            self.test_requires("benchmark/1.7.1")

    def layout(self):
        cmake_layout(self)

    def build(self):
        variables = {
            "ENABLE_ARCH": self.options.enable_arch,
            "ENABLE_BENCHMARKS": self.options.enable_benchmarks,
            "ENABLE_IO_URING": self.options.enable_io_uring
        }

        cmake = CMake(self)
        cmake.configure(variables=variables)
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
