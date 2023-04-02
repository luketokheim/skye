#
# Build on Alpine with g++ and musl/libc. Link statically for a portable binary.
# Run on scratch.
#
# docker build -t skye .
# docker run --rm -p 8080:8080 skye
#
FROM alpine:3.17 as builder

# Install build requirements from package repo.
RUN apk update && apk add --no-cache \
    cmake \
    g++ \
    linux-headers \
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

# Download dependencies and build example apps.
RUN conan build . --build=missing -o developer_mode=True

# Install.
RUN cmake --install build/Release --prefix build/install --strip

FROM scratch as runtime

#
# Allow user to choose which example to deploy from command line.
#
# docker build -t skye:echo --build-arg echo .
# docker build -t skye:producer --build-arg producer .
#
ARG appname=hello

COPY --from=builder /source/build/install/bin/skye-${appname} /skye

ENV PORT=8080

ENTRYPOINT ["/skye"]

EXPOSE $PORT
