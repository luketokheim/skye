#
# Build on Alpine with g++ and musl/libc. Link statically for a portable binary.
# Run on scratch.
#
# docker build -t skye .
# docker run --rm -p 8080:8080 skye
#
FROM alpine:3.17 as builder

# Install build requirements from package repo
RUN apk update && apk add --no-cache \
    cmake \
    g++ \
    linux-headers \
    make \
    ninja \
    py-pip

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
