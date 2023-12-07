#!/bin/bash

# Function to download and extract TI build tools
download_only() {
    local download_url="$1"
    local target_directory="$2"
    
    echo "Downloading to $target_directory..."
    mkdir -p "$target_directory"
    wget -q "$download_url" -P "$target_directory"  # Use -P to specify the target directory
    echo "Download complete to $target_directory"
}

# Function to download and extract TI build tools
download_and_extract_build_tools() {
    local toolchain_url="$1"
    local target_directory="$2"
    
    echo "Downloading and extracting TI build tools to $target_directory..."
    mkdir -p "$target_directory"
    wget -q "$toolchain_url" -O - | tar -xJ -C "$target_directory"
    echo "TI build tools extracted to $target_directory"
}

# Function to download and extract TI Processor SDK
download_and_extract_processor_sdk() {
    local sdk_url="$1"
    local target_directory="$2"
    
    echo "Downloading TI Processor SDK to $target_directory..."
    mkdir -p "$target_directory"
    wget -q "$sdk_url" -P "$target_directory"
    echo "TI Processor SDK downloaded to $target_directory"
    
    # Extract .gz file
    tar -xzf "$target_directory/$(basename $sdk_url)" -C "$target_directory"
    
}

download_and_make_executable_processor_sdk() {
    local sdk_url="$1"
    local target_directory="$2"
    
    echo "Downloading TI Processor SDK to $target_directory..."
    mkdir -p "$target_directory"
    wget -q "$sdk_url" -P "$target_directory"
    echo "TI Processor SDK downloaded to $target_directory"

    # Make .bin file executable
    chmod +x "$target_directory/$(basename $sdk_url)"
    echo "TI Processor SDK .bin file made executable in $target_directory"
}

# Specify the URLs for TI build tools and TI Processor SDKs
build_tools_linux_url="https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz"
build_tools_uboot_url="https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf.tar.xz"
processor_sdk_linux_url="https://dr-download.ti.com/software-development/software-development-kit-sdk/MD-U6uMjOroyO/08.00.00.08/ti-processor-sdk-linux-j7-evm-08_00_00_08-Linux-x86-Install.bin"
processor_sdk_rtos_url="https://dr-download.ti.com/software-development/software-development-kit-sdk/MD-bA0wfI4X2g/08.00.00.12/ti-processor-sdk-rtos-j721e-evm-08_00_00_12.tar.gz"
tisdk_default_image_url="https://dr-download.ti.com/software-development/software-development-kit-sdk/MD-U6uMjOroyO/08.00.00.08/tisdk-default-image-j7-evm.tar.xz"
tisdk_boot_image_url="https://dr-download.ti.com/software-development/software-development-kit-sdk/MD-U6uMjOroyO/08.00.00.08/boot-j7-evm.tar.gz"

# Specify the target directories for extraction
build_tools_directory="./ti-build-tools"
processor_sdk_directory="./ti-processor-sdk"

# Download and extract TI build tools
download_and_extract_build_tools "$build_tools_linux_url" "$build_tools_directory"
download_and_extract_build_tools "$build_tools_uboot_url" "$build_tools_directory"

# Download and extract TI Processor SDK
download_and_extract_processor_sdk "$processor_sdk_rtos_url" "$processor_sdk_directory"
download_and_make_executable_processor_sdk "$processor_sdk_linux_url" "$processor_sdk_directory"
download_only "$tisdk_default_image_url" "$processor_sdk_directory/ti-processor-sdk-rtos-j721e-evm-08_00_00_12"
download_only "$tisdk_boot_image_url" "$processor_sdk_directory/ti-processor-sdk-rtos-j721e-evm-08_00_00_12"
