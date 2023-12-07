#!/bin/bash

# Define paths
rtos_sdk_dir="ti-processor-sdk/ti-processor-sdk-rtos-j721e-evm-08_00_00_12"
linux_sdk_dir="ti-processor-sdk/ti-processor-sdk-linux-j7-evm-08_00_00_08"

# Check if the targetfs directory exists in the RTOS SDK directory
if [ -d "$rtos_sdk_dir/targetfs" ]; then
    # Delete the existing targetfs directory
    echo "Deleting existing targetfs directory..."
    rm -rf "$rtos_sdk_dir/targetfs"
fi

# Create a symbolic link from targetfs to targetNFS in the RTOS SDK directory
echo "Creating a symbolic link from targetfs to targetNFS..."
ln -s "$linux_sdk_dir/targetNFS" "$rtos_sdk_dir/targetfs"

echo "Symbolic link 'targetfs' created."
