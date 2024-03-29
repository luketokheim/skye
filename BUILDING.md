# Building with CMake

## Dependencies

### Boost (>=1.79)

Skye requires [Boost](https://www.boost.org/) and intends to support any
installation method that works with
[FindBoost](https://cmake.org/cmake/help/latest/module/FindBoost.html). This
library uses the header only modules Boost.Asio and Boost.Beast.

### fmt (>=9.0 optional)

Skye uses the [fmt](https://fmt.dev/) library in some examples and in the
[skye/format.hpp](include/skye/format.hpp) header.

### Test, examples, and benchmarks

For a list of dependencies, refer to [conanfile.py](conanfile.py).

## Build

This project doesn't require any special command-line flags to build to keep
things simple.

Here are the steps for building in release mode with a single-configuration
generator, like the Unix Makefiles one:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Here are the steps for building in release mode with a multi-configuration
generator, like the Visual Studio ones:

```sh
cmake -S . -B build
cmake --build build --config Release
```

### Building with MSVC

Note that MSVC by default is not standards compliant and you need to pass some
flags to make it behave properly. See the `flags-windows` preset in the
[CMakePresets.json](CMakePresets.json) file for the flags and with what
variable to provide them to CMake during configuration.

### Building on Apple Silicon

CMake supports building on Apple Silicon properly since 3.20.1. Make sure you
have the [latest version][1] installed.

## Install

This project doesn't require any special command-line flags to install to keep
things simple. As a prerequisite, the project has to be built with the above
commands already.

The below commands require at least CMake 3.15 to run, because that is the
version in which [Install a Project][2] was added.

Here is the command for installing the release mode artifacts with a
single-configuration generator, like the Unix Makefiles one:

```sh
cmake --install build
```

Here is the command for installing the release mode artifacts with a
multi-configuration generator, like the Visual Studio ones:

```sh
cmake --install build --config Release
```

### CMake package

This project exports a CMake package to be used with the [`find_package`][3]
command of CMake:

* Package name: `skye`
* Target name: `skye::skye`

Example usage:

```cmake
find_package(skye REQUIRED)
# Declare the imported target as a build requirement using PRIVATE, where
# project_target is a target created in the consuming project
target_link_libraries(
    project_target PRIVATE
    skye::skye
)
```

### Note to packagers

The `CMAKE_INSTALL_INCLUDEDIR` is set to a path other than just `include` if
the project is configured as a top level project to avoid indirectly including
other libraries when installed to a common prefix. Please review the
[install-rules.cmake](cmake/install-rules.cmake) file for the full set of
install rules.

[1]: https://cmake.org/download/
[2]: https://cmake.org/cmake/help/latest/manual/cmake.1.html#install-a-project
[3]: https://cmake.org/cmake/help/latest/command/find_package.html


## Conan package

This project includes a recipe that can be used to a conan package. Here is the
command to create a local package that you can use from your projects.

```console
conan create . --build=missing --version 0.12.0
```

Example `conanfile.txt` file for a project that uses this library:

```yaml
[requires]
skye/0.12.0

[generators]
CMakeDeps
CMakeToolchain

[layout]
cmake_layout
```
