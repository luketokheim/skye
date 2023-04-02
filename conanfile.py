from conan import ConanFile
from conan.tools.files import copy


class SkyeConan(ConanFile):
    name = "skye"
    description = "Skye is an HTTP server framework for C++20."
    homepage = "https://github.com/luketokheim/skye"
    license = "BSL-1.0"

    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv"
    exports_sources = "include/*"
    no_copy_source = True

    def layout(self):
        self.folders.generators = "conan"

    def requirements(self):
        self.requires("boost/1.81.0", options={"header_only": True})

    def build_requirements(self):
        self.test_requires("benchmark/1.7.1")
        self.test_requires("catch2/3.3.2")
        self.test_requires("fmt/9.1.0")
        self.test_requires("sqlite3/3.41.1")

    def package(self):
        copy(self, "*.hpp", self.source_folder, self.package_folder)