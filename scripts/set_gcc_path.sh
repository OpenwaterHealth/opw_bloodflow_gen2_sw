#!/bin/bash

# Get the current directory
CURR_DIR="$(pwd)"

# Set GCC92PATH
GCC92PATH="$CURR_DIR/ti-build-tools/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu/bin:$CURR_DIR/ti-build-tools/gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf/bin"

# Update the PATH variable
export PATH="$GCC92PATH:$PATH"

# Print a message indicating the variables have been set
echo "GCC92PATH and PATH have been configured."
