from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout


class SkyeRecipe(ConanFile):
    name = "skye"
    version = "0.10.0"
    description = "Skye is an HTTP server framework for C++20."
    homepage = "https://github.com/luketokheim/skye"
    license = "BSL"

    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeToolchain", "CMakeDeps"
    exports_sources = "CMakeLists.txt", "include/*", "examples/*", "tests/*"
    options = {"enable_io_uring": [True, False]}
    default_options = {"enable_io_uring": False}

    def requirements(self):
        self.test_requires("boost/1.81.0", options={"header_only": True})

        self.test_requires("fmt/9.1.0")

        self.test_requires("sqlite3/3.41.1")

        self.test_requires("catch2/3.3.2")

    def layout(self):
        cmake_layout(self)

    def build(self):
        variables = {"ENABLE_IO_URING": self.options.enable_io_uring}

        cmake = CMake(self)
        cmake.configure(variables=variables)
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
