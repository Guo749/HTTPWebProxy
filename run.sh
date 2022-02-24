#!/bin/bash
# used to boot the proxy

make clean
make -j8
./proxy 12345


# boot using docker
# allocating more cpu resources.
# docker run -d -it -p 12345:12345 --cpus="3.5" --name proxy --mount type=bind,source="$(pwd)",target=/code ubuntu:18.04