# ESP32 Mesh Project

This repository contains firmware for a mesh network of ESP32-C6 devices, supporting multiple hardware roles (network node, robot, RTK base, and development board). The project uses the ESP-IDF framework and PlatformIO for building and configuration.

## Setup Instructions

### 1. Install PlatformIO for VS Code
- Download and install [Visual Studio Code](https://code.visualstudio.com/).
- Open VS Code and go to the Extensions view (`Ctrl+Shift+X`).
- Search for **PlatformIO IDE** and install it.
- Restart VS Code if prompted.

### 2. Clone This Repository
Clone or download this repository to your local machine.

### 3. Open the Project in VS Code
- Open the folder containing this project (`platformio.ini`) in VS Code.
- PlatformIO should automatically detect the project.

### 4. Select and Build an Environment
There are **four build environments** defined in `platformio.ini`:
- `esp32-c6-net`   : Network node (`-DNET_NODE`)
- `esp32-c6-rtk`   : RTK base node (`-DRTK_BASE_NODE`)
- `esp32-c6-robot` : Robot node (`-DROBOT_NODE`)
- `dev_board`      : Generic development board (no special node flag)

Each environment sets specific build flags for the intended hardware role. Select the desired environment in the PlatformIO toolbar (bottom bar in VS Code) before building or uploading.

### 5. Configure Project Options
This project uses a configuration menu for custom options, defined in `src/Kconfig.projbuild`.

#### To configure:
- Open the PlatformIO sidebar.
- Click on **Project Tasks** > your environment > **menuconfig** (or run `pio run -t menuconfig` in the terminal).
- This opens a graphical configuration menu where you can adjust mesh, WiFi, battery, and board settings.
- When you save and exit, this will generate the appropriate `sdkconfig.*` file for your selected environment.

#### Main Configuration Options (from `Kconfig.projbuild`):
- **Mesh Topology**: Choose between tree or chain network structure.
- **Power Save Options**: Enable/disable mesh power saving and set duty cycles.
- **WiFi Antenna Selection**: Use internal or external antenna (for XIAO ESP32C6).
- **Mesh Network Parameters**: Set SSID, password, channel, max layer, routing table size, etc.
- **Authentication Modes**: Select WiFi authentication for mesh AP.
- **Battery Voltage Input Pin**: Select analog input pin for battery voltage measurement.
- **Board Type**: Choose between Network, Robot, or Base Station roles.

All configuration options are documented in `src/Kconfig.projbuild` with help text for each setting.

### 6. Build Environments
The `platformio.ini` defines four build environments:
- **esp32-c6-net**: Strictly mesh network nodes (`-DNET_NODE`).
- **esp32-c6-rtk**: RTK base station publishers (`-DRTK_BASE_NODE`).
- **esp32-c6-robot**: GPS RTK subscriber nodes (`-DROBOT_NODE`).
- **dev_board**: Generic development board (no special node flag).

Each environment sets a unique build flag for conditional compilation, targeting the intended hardware role. Select the correct environment in the PlatformIO toolbar before building or uploading.

## File Overview
- `platformio.ini`           : PlatformIO build environments and flags
- `src/Kconfig.projbuild`    : Project configuration menu (for menuconfig)
- `partitions_custom.csv`    : Custom partition table
- `sdkconfig.*`              : Example configuration files for different boards
- `src/`                     : Main source code
- `lib/`                     : Project libraries

## Building and Uploading
- Select the correct environment for your hardware.
- Use the PlatformIO toolbar or sidebar to build and upload the firmware.

## Notes
- Each build environment sets a unique flag for conditional compilation.
- Use `menuconfig` to customize mesh and board settings before building.
- For more details, see the comments in `Kconfig.projbuild` and `platformio.ini`.
