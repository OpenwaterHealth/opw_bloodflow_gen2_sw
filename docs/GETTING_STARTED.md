# Getting Started

Welcome to the Openwater Blood Flow Application! This guide will help you set up your development environment and get started with the project.

## Additional Resources

* [Texas Instruments - PROCESSOR-SDK-LINUX-J721E](https://software-dl.ti.com/jacinto7/esd/processor-sdk-linux-jacinto7/08_00_00_08/exports/docs/linux/Overview.html)
* [Texas Instruments - Processor SDK RTOS J721E](https://software-dl.ti.com/jacinto7/esd/processor-sdk-rtos-jacinto7/08_00_00_12/exports/docs/psdk_rtos/docs/user_guide/index.html)

### Prerequisites

* **Operating System**: Linux-based system (Ubuntu 18.04) or Windows 10 with WSL2 running Ubuntu 18.04.
* **TI Build Tools**:
  * [ARM64 CGT for A72/A53 Linux](https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz)
  * [ARM32 CGT for R5F U Boot](https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-arm-none-linux-gnueabihf.tar.xz)
* **TI Processor SDK**:
  * [RTOS version 08.00.00.12](https://dr-download.ti.com/software-development/software-development-kit-sdk/MD-bA0wfI4X2g/08.00.00.12/ti-processor-sdk-rtos-j721e-evm-08_00_00_12.tar.gz)
  * [Linux version 08.00.00.08](https://dr-download.ti.com/software-development/software-development-kit-sdk/MD-U6uMjOroyO/08.00.00.08/ti-processor-sdk-linux-j7-evm-08_00_00_08-Linux-x86-Install.bin)
* **Openwater TI Kernel and U-Boot**:
  * [linux-kernel-opw](https://github.com/OpenwaterHealth/ti-linux-kernel.git)
  * [u-boot-opw](https://github.com/OpenwaterHealth/ti-u-boot.git)
* **Python**: For running data analysis and visualization scripts.

### Setup and Installation

1. **Clone the Repository**:
   ```shell
   git clone git@github.com:OpenwaterHealth/opw_bloodflow_gen2_sw.git
   cd opw_bloodflow_gen2_sw
   ```

2. **Install Required Host Packages**:
   ```shell
   sudo apt-get install build-essential autoconf automake bison flex libssl-dev bc u-boot-tools
   ```

3. **Download and Extract the TI SDKs and Tools**:
   ```shell
   chmod +x scripts/download.sh
   ./scripts/download.sh
   ```

4. **Install TI Processor SDK Linux**:

   **Note**: The installation process for TI Processor SDK Linux involves a graphical user interface (UI). Please follow these steps to navigate the UI installation:

   * Run the following command to start the installation process:

     ```shell
     ./ti-processor-sdk/ti-processor-sdk-linux-j7-evm-08_00_00_08-Linux-x86-Install.bin --prefix "$(pwd)/ti-processor-sdk/ti-processor-sdk-linux-j7-evm-08_00_00_08"
     ```

   * The installer will launch a graphical UI. Follow the on-screen instructions to complete the installation.

   * Once the installation is complete, the TI Processor SDK Linux will be installed in the specified directory.

   If you encounter any issues during the installation process or have specific installation requirements, please refer to the official documentation or contact our support team for assistance.

5. **Replace U-Boot and Kernel with Openwater version**:
   ```shell
   chmod +x scripts/replace_kernel.sh
   ./scripts/replace_kernel.sh
   ```

6. **Setup TI Processor SDK Linux**:

   **Note**: The setup process for TI Processor SDK Linux involves a console interface. Please follow these steps to navigate the installation no changes to the defaults should be required:

   * Run the following command to start the setup process:
     ```shell
     ./ti-processor-sdk/ti-processor-sdk-linux-j7-evm-08_00_00_08/setup.sh
     ```
   * Once the installation is complete, the TI Processor SDK Linux will be setup in the specified directory.

   If you encounter any issues during the installation process or have specific installation requirements, please refer to the official documentation or contact our support team for assistance.

7. **Test Build Processor SDK Linux**:

   **Note**: For running on WSL2 you must ensure that your PATH variable is set to only linux paths something like below will work:

   ```shell
   # Get the current PATH variable
   current_path="$PATH"

   # Remove everything after and including the first instance of /mnt/c
   new_path=$(echo "$current_path" | sed 's|/mnt/c.*||')

   # Update the PATH variable
   export PATH="$new_path"

   # Print the updated PATH
   echo "Updated PATH: $PATH"
   ```

   * Run the following command to build the TI Processor Linux SDK:
     ```shell
     source ./scripts/set_gcc_path.sh
     cd ti-processor-sdk/ti-processor-sdk-linux-j7-evm-08_00_00_08/
     make clean
     make 
     make install

     # After build is complete cd back to ther root of the repository
     cd ../../
     ```

   After successfully building the TI Processor SDK Linux, the `targetNFS` directory will be updated with the latest build artifacts from the processor SDK Linux. These artifacts are essential for booting the Openwater hardware device.

   If you encounter any build errors or issues during this step, please refer to the TI Documentation in the additional resources section above or contact our support team for assistance.

8. **Setup TI Processor SDK RTOS**:
   ```shell
   cd ti-processor-sdk/ti-processor-sdk-rtos-j721e-evm-08_00_00_12/
   ./psdk_rtos/scripts/setup_psdk_rtos.sh 

   # After installation is complete cd back to ther root of the repository
   cd ../../
   ```

9. **Set the target filesystem common to both SDKs**:
   ```shell
   chmod +x scripts/common_targetfs.sh
   ./scripts/common_targetfs.sh
   ```

10. **Add Openwater configuration to filesystem**:

    ```shell
    chmod +x scripts/update_targetfs.sh
    ./scripts/update_targetfs.sh
    ```

11. **Test Build Processor SDK RTOS**:

    ```shell
    cd ti-processor-sdk/ti-processor-sdk-rtos-j721e-evm-08_00_00_12/vision_apps
    make sdk_check_paths
    make sdk
    make linux_fs_install
    ```

    After successfully building the TI Processor SDK RTOS, the `targetNFS` directory in the TI Processor SDK Linux will be updated with the latest build artifacts from the processor SDK RTOS. These artifacts are essential for Vision Applications to run.

    If you encounter any build errors or issues during this step, please refer to the TI Documentation in the additional resources section above or contact our support team for assistance.

12. **Add Openwater TI Processor SDK Modifications**:

    ```shell
    chmod +x scripts/update_sdk.sh
    ./scripts/update_sdk.sh
    ```

13. **Build Image with Application**:

    ```shell
    cd ti-processor-sdk/ti-processor-sdk-rtos-j721e-evm-08_00_00_12/vision_apps
    make sdk_check_paths
    make sdk
    make linux_fs_install
    ```

    After successfully building the `targetNFS` directory in the TI Processor SDK Linux will be updated with the latest build artifacts from the processor SDK RTOS. These artifacts will now include the Openwater Blood Flow application.

    **Note**: The Himax Camera sensor registers have been redacted due to NDA please contact our support team for assistance.

    If you encounter any build errors or issues during this step, please refer to the TI Documentation in the additional resources section above or contact our support team for assistance.

14. **Building The User Interface**:

    The user interface for the Openwater Blood Flow system is built using QT. Refer to the [GEN2 Display documentation](https://github.com/OpenwaterHealth/opw_bloodflow_gen2_sw/tree/main/bloodflow/gen2display) for details on how to build the UI code from source.

15. **Creating The SDCard Image**:

    Use the scripts found in the `psdk_rtos/scripts` folder to make and install to SDCard for booting.

