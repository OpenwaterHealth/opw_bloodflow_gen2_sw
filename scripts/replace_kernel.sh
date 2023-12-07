#!/bin/bash

# Specify the paths to the kernel and U-Boot source directories
kernel_source_path="ti-processor-sdk/ti-processor-sdk-linux-j7-evm-08_00_00_08/board-support/linux-5.10.41+gitAUTOINC+4c2eade9f7-g4c2eade9f7"
uboot_source_path="ti-processor-sdk/ti-processor-sdk-linux-j7-evm-08_00_00_08/board-support/u-boot-2021.01+gitAUTOINC+53e79d0e89-g53e79d0e89"

# Specify the path to the Rules.make file
rules_make_file="ti-processor-sdk/ti-processor-sdk-linux-j7-evm-08_00_00_08/Rules.make"

# Old and new values
OLD_VALUE="linux-5.10.41+gitAUTOINC+4c2eade9f7-g4c2eade9f7"
NEW_VALUE="linux-kernel-opw"

# Function to delete a directory if it exists
delete_directory() {
    local dir="$1"
    if [ -d "$dir" ]; then
        echo "Deleting $dir..."
        rm -r "$dir"
        echo "Deleted $dir"
    else
        echo "$dir does not exist. No action taken."
    fi
}

# Function to clone repositories
clone_repositories() {
    local kernel_repo_url="https://github.com/OpenwaterInternet/ti-linux-kernel.git"
    local uboot_repo_url="https://github.com/OpenwaterInternet/ti-u-boot.git"
        
    echo "Cloning TI Linux Kernel into linux-kernel-opw..."
    git clone "$kernel_repo_url" ti-processor-sdk/ti-processor-sdk-linux-j7-evm-08_00_00_08/board-support/linux-kernel-opw
    
    echo "Cloning U-Boot into u-boot-opw..."
    git clone "$uboot_repo_url" ti-processor-sdk/ti-processor-sdk-linux-j7-evm-08_00_00_08/board-support/u-boot-opw
}

# Function to update Rules.make
update_rules_make() {
    local file="$1"
    local old_value="$2"
    local new_value="$3"
    
    # Use sed to replace the old value with the new value
    sed -i "s|$old_value|$new_value|g" "$file"
    
    echo "Rules.make updated successfully."
}

# Delete the specified kernel and U-Boot source directories (if they exist)
delete_directory "$kernel_source_path"
delete_directory "$uboot_source_path"

# Clone the required repositories
clone_repositories

# Update the Rules.make file
update_rules_make "$rules_make_file" "$OLD_VALUE" "$NEW_VALUE"
