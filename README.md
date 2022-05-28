# Http Microservice for C++

[![sanitizer](https://github.com/luketokheim/httpmicroservice-cpp/actions/workflows/sanitizer.yaml/badge.svg)](https://github.com/luketokheim/httpmicroservice-cpp/actions/workflows/sanitizer.yaml)
[![test](https://github.com/luketokheim/httpmicroservice-cpp/actions/workflows/test.yaml/badge.svg)](https://github.com/luketokheim/httpmicroservice-cpp/actions/workflows/test.yaml)

Run your C++ function as a containerized web service.

```console
docker build -t httpmicroservice-cpp .
```

```console
docker run --rm -p 8080:8080 httpmicroservice-cpp
```

## Design

The basic idea is that you use this library to run your C++ function in response
to an HTTP request. The library provides the server functionality and handles the
networking and protocol aspects for you.

The service runs one thread for all network I/O. For blocking or long running
synchronous tasks inside your C++ function your must provide a worker thread
pool to prevent blocking the networking coroutines.

This service is intended to run behind a reverse proxy that terminates TLS and
maps requests to this application. I am using it on Google Cloud Run but other
containerized environments will work.

Since this service assumes it is running behind a reverse proxy there are some
features that I omitted.

- No SSL/TLS support
- No request or keep alive timeouts
- No request target to handler mapping
- No static content or nice error pages

## Requirements

This project is a C++20 library that uses coroutines for network I/O. The use
of coroutines is inspired by the [Talking Async Ep1: Why C++20 is the Awesomest
Language for Network Programming](https://youtu.be/icgnqFM-aY4) video by Chris
Kohlhoff.

- [Coroutines](https://en.cppreference.com/w/cpp/language/coroutines) support from a modern compiler
- [Boost.Asio](https://think-async.com/Asio/) for network I/O
- [Boost.Beast](https://github.com/boostorg/beast) to parse HTTP requests and form reponses

## Compilers

This project requires C++20 support for coroutines.

- Microsoft Visual Studio 2022
- Clang 13
- G++ 10

## Package managers

This project works with the [conan](https://conan.io/) and
[vcpkg](https://vcpkg.io/) C++ package managers. I use the conan build for CI
and Docker images because it builds faster on Linux with the package
dependencies.
