#
# docker build -t httpmicroservice-cpp .
# docker run --rm -p 8080:8080 httpmicroservice-cpp
#
FROM alpine:latest as builder

# Install build requirements from package repo
RUN apk update && apk add --no-cache \
    cmake \
    g++ \
    liburing-dev \
    linux-headers \
    make \
    ninja \
    py-pip

# Install conan package manager
RUN pip install conan --upgrade && conan profile new default --detect

# Copy repo source code
COPY . /source

# Run cmake commands from the build folder
WORKDIR /source/build

# Download dependencies and generate cmake toolchain file
RUN conan install .. --build=missing

# Configure, build with static musl/libc and libstdc++ so we can run on the
# scratch empty base image
RUN cmake .. -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
    -DENABLE_STANDALONE=ON \
    -DENABLE_IO_URING=ON \
    -DBUILD_TESTING=OFF

# Build
RUN cmake --build .

# Install
RUN cmake --install . --strip

FROM scratch as runtime

COPY --from=builder /usr/local/bin/usrv-hello /usrv

ENV PORT=8080

ENTRYPOINT ["/usrv"]

EXPOSE $PORT
