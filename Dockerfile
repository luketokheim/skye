#
# docker build -t httpappserver-cpp .
#
FROM alpine:latest as builder

# Install build requirements from package repo
RUN apk update && apk add --no-cache \
    cmake \
    g++ \
    linux-headers \
    make \
    py-pip

# Install conan package manager
RUN pip install conan && conan profile new default --detect

# Copy repo source code
COPY . /httpappserver-cpp

# Run cmake commands from the build folder
WORKDIR /httpappserver-cpp/build

# Download dependencies and generate cmake toolchain file
RUN conan install .. --build=missing

# Configure, build with static musl/libc and libstdc++ so we can run on the
# scratch empty base image
RUN cmake -B . -S .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
    -DCMAKE_CXX_FLAGS="-static-libstdc++ -static" \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libstdc++ -static"

# Build
RUN cmake --build . --config Release

# Run unit tests
# RUN ctest -C Release

# Install 
RUN cmake --install . --config Release

FROM scratch as runtime

COPY --from=builder /usr/local/bin/cli /cli

ENTRYPOINT ["/cli"]

EXPOSE 8080
