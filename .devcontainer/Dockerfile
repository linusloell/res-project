FROM mcr.microsoft.com/devcontainers/cpp:ubuntu-24.04

# The base image already ships gcc, make, gdb, valgrind and cppcheck.
# Install any additional tooling here (e.g. cross-compilers) as the project grows.
USER root
RUN apt-get update \
    && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
        clang-tidy \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*
