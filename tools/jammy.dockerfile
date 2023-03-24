#
# Build on Ubuntu 22.04 LTS (Jammy Jellyfish) with g++ and libstdc++. Link
# statically so we have a portable binary. Run on scratch.
#
# docker build -t skye -f tools/jammy.dockerfile .
# docker run --rm -p 8080:8080 skye
#
FROM ubuntu:jammy as builder

# Install build requirements from package repo
RUN apt-get update && apt-get install -y \
    cmake \
    g++-12 \
    make \
    ninja-build \
    python3-pip

# Install conan package manager
RUN pip install conan && conan profile detect

# Copy repo source code
COPY . /source

# Run cmake commands from the build folder
WORKDIR /source

# Populate conan "global.conf" file for standalone builds.
RUN cat tools/standalone.conf >> ~/.conan2/global.conf

# Download dependencies and generate cmake toolchain file.
RUN conan install . --build=missing -o with_sqlite3=True

# Calls cmake. Build with static libc and libstdc++ so the apps run on the
# scratch empty base image.
RUN conan build . -o enable_standalone=True

# Install.
RUN cmake --install build/Release --strip --verbose

FROM scratch as runtime

ARG appname=hello

COPY --from=builder /source/bin/skye-${appname} /skye

ENV PORT=8080

ENTRYPOINT ["/skye"]

EXPOSE $PORT
