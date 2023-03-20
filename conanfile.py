from conan import ConanFile
from conan.tools.cmake import cmake_layout


class SkyeRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

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

    def requirements(self):
        if self.options.with_boost:
            self.requires("boost/1.81.0")

        if self.options.with_fmt:
            self.requires("fmt/9.1.0")

        if self.options.with_sqlite3:
            self.requires("sqlite3/3.41.1")

        if not self.conf.get("tools.build:skip_test", default=False):
            self.requires("catch2/3.3.2")

    def build_requirements(self):
        # self.tool_requires("cmake/3.19.8")
        pass

    def layout(self):
        cmake_layout(self)
