# Openwater Blood Flow Application Repository

## Overview

The Openwater Blood Flow Application repository is a comprehensive suite containing source code for the Gen2 Blood Flow Analysis Vision Application, including the vision application itself, DSP kernel for real-time image data processing, User Interface, and Python modules for data analysis and visualization.

## Repository Structure

### Vision Application

- [/bloodflow/app_openwater](/bloodflow/app_openwater): Code for the vision application.

### DSP Kernel

- [/bloodflow/ow_algos](/bloodflow/ow_algos): DSP algorithm implementations for image analysis.

### User Interface

- [/bloodflow/gen2display](/bloodflow/gen2display): UI application code.

### Python Data Analysis Module

- [/bloodflow/app_python_modules](/bloodflow/app_python_modules): Python module for analyzing collected data and generating visualizations.

### Scripts

- [/scripts](/scripts): Scripts directory that contains helper scripts for environment setup and building.

### Build Tools

- [/ti-build-tools](/ti-build-tools): Required build tools for building the system components.

### TI Processor SDKs

- [/ti-processor-sdk](/ti-processor-sdk): TI TDA4VM processor SDK directory.
  
## Getting Started

### Prerequisites

- **Operating System**: Linux-based system (Ubuntu 18.04) or Windows 10 with WSL2 running Ubuntu 18.04.
- **TI Build Tools**: 
  - [ARM64 CGT for A72/A53 Linux](https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz)
  - [ARM32 CGT for R5F U Boot](https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf.tar.xz)
- **TI Processor SDK**: 
  - [RTOS version 08.00.00.12](https://dr-download.ti.com/software-development/software-development-kit-sdk/MD-bA0wfI4X2g/08.00.00.12/ti-processor-sdk-rtos-j721e-evm-08_00_00_12.tar.gz)
  - [Linux version 08.00.00.08](https://dr-download.ti.com/software-development/software-development-kit-sdk/MD-U6uMjOroyO/08.00.00.08/ti-processor-sdk-linux-j7-evm-08_00_00_08-Linux-x86-Install.bin)
- **Python**: For running data analysis and visualization scripts.

### Setup and Installation

1. **Clone the Repository**:
   ```shell
   git clone git@github.com:OpenwaterInternet/opw_bloodflow_gen2_sw.git
   cd opw_bloodflow_gen2_sw

2. **Install Required Host Packages**:
   ```shell
   sudo apt-get install build-essential autoconf automake bison flex libssl-dev bc u-boot-tools

3. **Download and Extract the TI SDKs and Tools**:
   ```shell
   chmod +x scripts/download.sh
   ./scripts/download.sh

4. **Install TI Processor SDK Linux**:

   **Note**: The installation process for TI Processor SDK Linux involves a graphical user interface (UI). Please follow these steps to navigate the UI installation:

   - Run the following command to start the installation process:

     ```shell
     ./ti-processor-sdk/ti-processor-sdk-linux-j7-evm-08_00_00_08-Linux-x86-Install.bin --prefix "$(pwd)/ti-processor-sdk/ti-processor-sdk-linux-j7-evm-08_00_00_08"
     ```

   - The installer will launch a graphical UI. Follow the on-screen instructions to complete the installation. 

   - Once the installation is complete, the TI Processor SDK Linux will be installed in the specified directory.

   If you encounter any issues during the installation process or have specific installation requirements, please refer to the official documentation or contact our support team for assistance.

