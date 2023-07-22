FROM ubuntu:latest

RUN apt-get update
RUN apt-get upgrade -y

RUN apt-get install -y \
    mono-complete \
    cmake \
    ninja-build \
    mingw-w64 \
    ;
