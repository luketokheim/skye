#
# docker build -t httpappserver-alpine .
#
FROM alpine:latest

# Install build dependencies from package repo
RUN apk update && apk add \
    cmake \
    g++ \
    linux-headers \
    py-pip

# Install conan package manager
RUN pip install conan && conan profile new default --detect
