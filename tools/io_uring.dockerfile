#
# Build on Alpine with g++, musl, and libstdc++. Link statically so we have a
# portable binary. Run on scratch.
#
# Enables io_uring for the socket I/O requests. Your target system must support
# the io_uring interface.
#
# docker build -t skye -f tools/io_uring.dockerfile .
# docker run --rm -p 8080:8080 skye
#
FROM alpine:3.17 as builder

# Install build requirements from package repo.
RUN apk update && apk add --no-cache \
    cmake \
    g++ \
    linux-headers \
    liburing-dev \
    make \
    ninja \
    py-pip

# Install conan package manager.
RUN pip install conan && conan profile detect

# Copy repo source code.
COPY . /source

# Run cmake commands from the build folder.
WORKDIR /source

# Populate conan "global.conf" file for standalone builds.
RUN cat tools/standalone.conf >> ~/.conan2/global.conf

# Download dependencies and generate cmake toolchain file.
RUN conan install . --build=missing -o with_sqlite3=True

# Calls cmake. Build with static musl/libc and libstdc++ so the apps run on the
# scratch empty base image.
RUN conan build . -o enable_standalone=True -o enable_io_uring=True

# Install.
RUN cmake --install build/Release --strip --verbose

FROM scratch as runtime

ARG appname=hello

COPY --from=builder /source/bin/skye-${appname} /skye

ENV PORT=8080

ENTRYPOINT ["/skye"]

EXPOSE $PORT
