# Skye

[![sanitizer](https://github.com/luketokheim/skye/actions/workflows/sanitizer.yaml/badge.svg)](https://github.com/luketokheim/skye/actions/workflows/sanitizer.yaml)
[![test](https://github.com/luketokheim/skye/actions/workflows/test.yaml/badge.svg)](https://github.com/luketokheim/skye/actions/workflows/test.yaml)
[![tidy](https://github.com/luketokheim/skye/actions/workflows/tidy.yaml/badge.svg)](https://github.com/luketokheim/skye/actions/workflows/tidy.yaml)

Skye is an HTTP server framework for C++20. Build resource friendly web services
for the cloud.

The framework is an example use case of the excellent [Asio](https://think-async.com/Asio/) and
[Boost.Beast](https://github.com/boostorg/beast) libraries. Out of the box you get:

- HTTP/1
- Asynchronous model
- Performance
- Small footprint

## Quick start

A minimal service needs a request handler.

```cpp
#include <skye/service.hpp>

namespace asio = boost::asio;
namespace http = boost::beast::http;

asio::awaitable<skye::response> hello_world(skye::request req)
{
    skye::response res{http::status::ok, req.version()};
    res.set(http::field::content_type, "text/plain");
    res.body() = "Hello World!";

    co_return res;
}
```

And a main function.

```cpp
int main()
{
    // Listen on port 8080 and route all HTTP requests to the hello_world handler
    skye::run(8080, hello_world);

    return 0;
}
```

Asio has excellent docs. Refer to those for more details on
[Basic Asio Anatomy](https://think-async.com/Asio/asio-1.26.0/doc/asio/overview/basics.html)
and [C++20 Coroutines Support](https://think-async.com/Asio/asio-1.26.0/doc/asio/overview/composition/cpp20_coroutines.html).

The framework uses [Boost.Beast](https://www.boost.org/doc/libs/release/libs/beast/doc/html/index.html)
types directly in its public interface. The intent is to allow users to do simple things easily
while still offering access to more advanced functionality.

## Docker

Build a Docker image that runs the [Hello World](examples/hello.cpp) web service.

```console
docker build -t skye .
```

Run the container.

```console
docker run --rm -p 8080:8080 skye
```

The image is based on the empty Docker "scratch" image and only contains the
single binary [Hello World](examples/hello.cpp) example server.

## Design

The basic idea is that you use this library to call your C++ function in response
to an HTTP request. The library provides the server functionality and handles the
networking and protocol aspects for you.

The service runs one thread for all network I/O. For blocking or long running
synchronous tasks inside your C++ function your may want to provide a worker
thread (or pool) to keep the main event loop running and processing other requests.

The service is intended to run behind a reverse proxy that terminates TLS and
maps requests to this application. The framework was prototyped on Google Cloud
Run but other container environments will work.

Since the service assumes it is running behind a reverse proxy there are some
features that were omitted.

- No SSL/TLS support
- No request or keep alive timeouts
- No request target resource to handler mapping
- No static content or nice error pages

## Asynchronous model

The framework uses only asynchronous operations. If you have used Asio before then
this main function will look more familiar.

```cpp
int main()
{
    asio::io_context ioc;

    // Listen on port 8080 and route all HTTP requests to the hello_world handler
    skye::async_run(ioc, 8080, hello_world);

    // Run event processing loop
    ioc.run();

    return 0;
}
```

By default, the framework calls `io_context::run()` from a single thread.

Request handlers run in a coroutine and may initiate their own asynchronous 
operations. Here is an example with a timer.

```cpp
asio::awaitable<skye::response> handler(skye::request req)
{
    auto ex = co_await asio::this_coro::executor;

    // Wait for 0.25 seconds without blocking other async operations.
    asio::steady_timer timer{ex, 250ms};
    co_await timer.async_wait(asio::use_awaitable);
    
    co_return skye::response{http::status::no_content, req.version()};
}
```

Boost has recently added client libraries for [MySQL](https://github.com/boostorg/mysql)
and [Redis](https://github.com/boostorg/redis) that support the asynchronous model.

## Requirements

This project is a C++20 library that uses coroutines for network I/O. The use
of coroutines is inspired by the [Talking Async Ep1: Why C++20 is the Awesomest
Language for Network Programming](https://youtu.be/icgnqFM-aY4) video by Chris
Kohlhoff.

- [Coroutines](https://en.cppreference.com/w/cpp/language/coroutines) support from a modern compiler
- [Asio](https://think-async.com/Asio/) for network I/O
- [Boost.Beast](https://github.com/boostorg/beast) to parse HTTP requests and form responses
- [Catch2](https://github.com/catchorg/Catch2) to run tests for continuous integration

For production use I recommend using io_uring (liburing-dev) on Linux if available. Enable it with
the `ENABLE_IO_URING` CMake option. The Docker and Continuous Deployment (CD) builds do not
install that library to maximize compatibility.

Cloud Run [second generation](https://cloud.google.com/run/docs/about-execution-environments)
execution environment supports io_uring but the managed container runtimes on
AWS (App Runner) and Azure (Container Apps) do not.

## Package manager

This project uses the [Conan](https://conan.io/) C++ package manager for Continuous Integration
(CI) and to build Docker images.

## Build

Create a build folder and install dependencies with the package manager.

```console
conan install . --build=missing
```

Use the toolchain file created by the package manager so cmake can locate
libraries with [find_package](https://cmake.org/cmake/help/latest/command/find_package.html).

```console
conan build .
```

Run tests.

```console
cd build/Release
ctest -C Release
```

## Compilers

This project requires C++20 support for coroutines. It runs on Windows, macOS,
and Linux.

- Microsoft Visual Studio 2022
- Clang 13
- G++ 10

## Why?

Why make another HTTP server library? Why should I use it?

The project started as an internal example or template for how to set up a Beast web service
that uses C++20 coroutines. Beast is a low-level framework with excellent features and options.
I did my best to choose the most appropriate and performant of those options for a cloud based web
service.

As the project developed I became envious of web frameworks in other languages like
Go `net/http` and Rust `hyper`. I tried to incorporate the simplicity of the usage of those
libraries into the interface to Skye.

You should use this framework if you:

- Want a server that cooperates with other Asio libraries.
- Want to use C++ to create a container app that you deploy to the cloud.

Do not use this framework to:

- Make a general purpose web server or serve files. Use nginx!
- Make a public facing web server on the internet. No TLS, no timeouts, HTTP/1 only.
