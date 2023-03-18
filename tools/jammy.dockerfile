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
WORKDIR /source/build

# Download dependencies and generate cmake toolchain file
RUN conan install .. --output-folder=. --build=missing

# Configure, build with static musl/libc and libstdc++ so we can run on the
# scratch empty base image
RUN cmake .. -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
    -DENABLE_STANDALONE=ON \
    -DENABLE_IO_URING=OFF \
    -DBUILD_TESTING=OFF

# Build
RUN cmake --build .

# Install
RUN cmake --install . --strip

FROM scratch as runtime

COPY --from=builder /usr/local/bin/skye-hello /skye

ENV PORT=8080

ENTRYPOINT ["/skye"]

EXPOSE $PORT
