#
# Database example. Creates a sqlite3 db and copies it into the image.
#
# Build on Alpine with g++ and musl/libc. Link statically for a portable binary.
# Run on scratch.
#
# docker build -t skye:database .
# docker run --rm -p 8080:8080 skye:database
#
FROM alpine:3.17 as builder

# Install build requirements from package repo.
RUN apk update && apk add --no-cache \
    cmake \
    g++ \
    linux-headers \
    make \
    ninja \
    py-pip \
    sqlite

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
RUN conan build . -o enable_standalone=True

# Install.
RUN cmake --install build/Release --strip --verbose

# Create the sqlite3 database.
RUN python tools/sqlite3_schema.py | sqlite3 bin/database.db

FROM scratch as runtime

ARG appname=database

COPY --from=builder /source/bin/skye-${appname} /skye

COPY --from=builder /source/bin/database.db /database.db

ENV PORT=8080

ENTRYPOINT ["/skye"]

EXPOSE $PORT