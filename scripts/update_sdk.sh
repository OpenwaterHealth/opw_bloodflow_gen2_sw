#!/bin/bash

# Store the current working directory in a variable
current_directory="$PWD"

source_directory="$current_directory/ti-processor-sdk/opw-processor-sdk/ti-processor-sdk-rtos-j721e-evm-08_00_00_12"
destination_directory="$current_directory/ti-processor-sdk/ti-processor-sdk-rtos-j721e-evm-08_00_00_12"

# Function to link code directories from this repo for building
link_code_directories() {
    echo "Linking app_openwater..."
    if ! cd ti-processor-sdk/ti-processor-sdk-rtos-j721e-evm-08_00_00_12/vision_apps/apps/basic_demos; then
        echo "Error: Unable to change directory."
        exit 1
    fi
    
    if ! ln -s "$current_directory/bloodflow/app_openwater" ./app_openwater; then
        echo "Error: Unable to create symbolic link for app_openwater."
        cd "$current_directory"
        exit 1
    fi
    
    cd "$current_directory"

    echo "Linking ow_algos..."
    if ! cd ti-processor-sdk/ti-processor-sdk-rtos-j721e-evm-08_00_00_12/vision_apps/kernels; then
        echo "Error: Unable to change directory."
        exit 1
    fi

    if ! ln -s "$current_directory/bloodflow/ow_algos" ./ow_algos; then
        echo "Error: Unable to create symbolic link for ow_algos."
        cd "$current_directory"
        exit 1
    fi
    
    cd "$current_directory"

}

# Function to clone repositories
clone_repositories() {
    local sdk_repo_url="https://github.com/OpenwaterHealth/opw-processor-sdk.git"
            
    echo "Cloning Openwater TI Processor SDK changes into ti-processor-sdk..."
    if ! git clone "$sdk_repo_url" ti-processor-sdk/opw-processor-sdk; then
        echo "Error: Unable to clone the repository."
        exit 1
    fi
    
    echo "Updating TI Processor SDK RTOS with changes..."
    cp -r "$source_directory/imaging"  "$destination_directory/"
    cp -r "$source_directory/vision_apps"  "$destination_directory/"
    cp -r "$source_directory/tiovx"  "$destination_directory/"
    cp -r "$source_directory/remote_device"  "$destination_directory/"
    cp -r "$source_directory/pdk_jacinto_08_00_00_37"  "$destination_directory/"
}

# Clone the required repositories
clone_repositories

# Update the Rules.make file
link_code_directories 
