# Haltech ESP32 Dash Display

This project implements a custom dash display and keypad interface for Haltech ECUs using an ESP32 microcontroller, ILI9488 touchscreen, and a custom PCB. The display communicates with the Haltech ECU over CAN bus.

## Features

- Real-time display of engine parameters
- Touchscreen interface for ECU controls
- Custom PCB design for compact integration
- CAN bus communication with Haltech ECU
- Configurable display layouts and themes

## Hardware Requirements

- ESP32 development board
- ILI9488 touchscreen display
- Custom PCB
- CAN bus transceiver (e.g., MCP2515)

## Software Dependencies

This project is built using PlatformIO. The following libraries are required:

- TFT_eSPI
- sandeepmistry/CAN

You can install these libraries through the PlatformIO Library Manager.

## Setup and Configuration

1. Clone this repository
2. Open the project in PlatformIO

## Building and Flashing

To build and flash the project to your ESP32:

1. Connect your ESP32 to your computer
2. In PlatformIO, click on the "Build" button to compile the project
3. Click on the "Upload" button to flash the firmware to your ESP32

## Usage

After flashing the firmware, the display will initialize and start communicating with the Haltech ECU over CAN. Use the touchscreen to interact with the display and send commands to the ECU.