#!/bin/bash

sudo apt update
sudo apt install \
    libxrandr-dev \
    libxcursor-dev \
    libxi-dev \
    libudev-dev \
    libfreetype-dev \
    libflac-dev \
    libvorbis-dev \
    libgl1-mesa-dev \
    libegl1-mesa-dev \
    libfreetype-dev \
    cmake \
    unzip

wget https://www.sfml-dev.org/files/SFML-3.0.0-sources.zip

unzip SFML-3.0.0-sources.zip

cmake -B build
cmake --build build
