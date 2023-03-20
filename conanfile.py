from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout


class SkyeRecipe(ConanFile):
    name = "skye"
    version = "0.9.0-alpha"
    description = "Skye is an HTTP microservice framework for C++20"
    homepage = "https://github.com/luketokheim/skye"
    license = "BSL"

    # Binary configuration
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "with_boost": [True, False],
        "with_fmt": [True, False],
        "with_sqlite3": [True, False]
    }
    default_options = {
        "with_boost": True,
        "with_fmt": True,
        "with_sqlite3": False,
        "boost*:header_only": True
    }

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "examples/*", "include/*", "src/*", "tests/*"

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        if self.options.with_boost:
            self.requires("boost/1.81.0")

        if self.options.with_fmt:
            self.requires("fmt/9.1.0")

        if self.options.with_sqlite3:
            self.requires("sqlite3/3.41.1")

        if not self.conf.get("tools.build:skip_test", default=False):
            self.requires("catch2/3.3.2")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    #def package_info(self):
    #    self.cpp_info.libs = ["skye"]
