# Openwater Blood Flow Application Repository

## Overview

The Openwater Blood Flow Application repository is a comprehensive suite containing source code for the Gen2 Blood Flow Analysis Vision Application, including the vision application itself, DSP kernel for real-time image data processing, User Interface, and Python modules for data analysis and visualization. For a more detailed overview, please visit our [Wiki](https://github.com/OpenwaterInternet/opw_bloodflow_gen2_sw/wiki).

## Software Architecture
For a detailed understanding of the software architecture of this project, please refer to the [Software Architecture Guide](ARCHITECTURE.md).

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

### Target Filesystem Configuration

- [/targetNFS](/targetNFS): configuration and additional packages needed for the Linux root filesystem.

### Build Tools

- [/ti-build-tools](/ti-build-tools): Required build tools for building the system components.

### TI Processor SDKs

- [/ti-processor-sdk](/ti-processor-sdk): TI TDA4VM processor SDK directory.
  

## Getting Started

For detailed instructions on setting up your development environment and getting started with this project, please refer to the [Getting Started Guide](GETTING_STARTED.md).