#!/bin/bash

# Define paths
linux_sdk_dir="ti-processor-sdk/ti-processor-sdk-linux-j7-evm-08_00_00_08"
sudo chown -R "$USER:$USER" "$linux_sdk_dir/targetNFS"
rm "$linux_sdk_dir/targetNFS/var/log"
cp -r targetNFS "$linux_sdk_dir/"
echo "Linux SDK 'targetfs' updated."