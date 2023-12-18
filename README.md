# Openwater Blood Flow Application Repository

## Overview

The Openwater Blood Flow Analysis Device is an innovative diagnostic tool designed to non-invasively measure and monitor cerebral blood flow. Utilizing advanced near-infrared light techniques, it facilitates the detection of hemodynamic changes associated with various medical conditions, particularly cerebrovascular events like strokes.

The Openwater Blood Flow Application repository is a comprehensive suite containing source code for the Gen2 Blood Flow Analysis Vision Application, including the vision application itself, DSP kernel for real-time image data processing, User Interface, and Python modules for data analysis and visualization. For a more detailed overview, please visit our [Wiki](https://wiki.openwater.health/index.php/Main_Page).

## Technology Description

Openwater Stroke Diagnosis Technology [Technology Paper](https://wiki.openwater.health/index.php/Openwater_Stroke_Diagnosis_Technology)

## Hardware Reference

For a detailed understanding of the hardware used by this software repository, please refer to the [Hardware Documentation](https://github.com/OpenwaterHealth/opw_bloodflow_gen2_hw)

## Software Architecture
For a detailed understanding of the software architecture of this project, please refer to the [Software Architecture Guide](https://wiki.openwater.health/index.php/Blood_Flow_Gen_2_Software_Architecture).

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

- [/ti-build-tools](/ti-build-tools): This directory serves as a place holder for required build tools for building the system components.

### TI Processor SDKs

- [/ti-processor-sdk](/ti-processor-sdk): TI TDA4VM processor SDK directory. This directory serves as a place holder where we will download the TI Processor SDKs which provides the Linux and RTOS components for the system that our application runs on.
  

## Getting Started

For detailed instructions on setting up your development environment and getting started with this project, please refer to the [Getting Started Guide](docs/GETTING_STARTED.md).


## Contribution Guide

For more information on how to contribute to the project, please refer to the [Contribution Guide](CONTRIBUTING.md).

## Investigational Use Only
CAUTION - Investigational device. Limited by Federal (or United States) law to investigational use. opw_bloodflow_gen2_sw has *not* been evaluated by the FDA and is not designed for the treatment or diagnosis of any disease. It is provided AS-IS, with no warranties. User assumes all liability and responsibility for identifying and mitigating risks associated with using this software.
