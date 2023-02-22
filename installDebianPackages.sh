#!/bin/bash

# Run this script to install any required Debian packages to run the assignment.

sudo apt update

# Install g++ cross-compiler. Needed to compile C++ source code.
sudo apt install -y g++-arm-linux-gnueabihf