# ESP32 LILYGO® T-Display-S3 Bitcoin Ticker

This project is a portable Bitcoin Ticker built using the **LILYGO® T-Display-S3** ESP32-S3 development board. It retrieves the latest Bitcoin (BTC) prices and displays them on the built-in TFT screen, making it a compact, convenient way to monitor BTC prices in real-time.

## Table of Contents
- [Overview](#overview)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Additional Files](#additional-files)
- [License](#license)

## Overview

This Bitcoin Ticker:
- Uses an ESP32-S3 board with a built-in TFT display.
- Connects to a Wi-Fi network to fetch live Bitcoin price data.
- Updates and displays the current BTC price on the screen in real-time.

## Hardware Requirements

- **LILYGO® T-Display-S3 ESP32-S3** development board
- USB-C cable for power and data transfer

## Software Requirements

1. **Arduino IDE** (or PlatformIO)
2. **ESP32 Board Support**: Install the ESP32 board package in the Arduino IDE.
3. **Required Libraries**:
   - [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) (for the display)
   - [WiFi](https://www.arduino.cc/reference/en/libraries/wifi/) (for Wi-Fi connectivity)
   - [HTTPClient](https://www.arduino.cc/reference/en/libraries/httpclient/) (for making HTTP requests)
   - Any additional libraries specific to the API used to fetch Bitcoin prices

## Installation

1. **Set up the Arduino IDE**:
   - Install the ESP32 board by going to **File > Preferences**, and adding the following URL to the **Additional Board Manager URLs**:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to **Tools > Board > Board Manager**, search for "ESP32", and install the latest version.
   - Select the **ESP32-S3 Dev Module** as the board under **Tools > Board**.

2. **Install Libraries**:
   - Go to **Tools > Manage Libraries** and install each of the required libraries listed above.

3. **Upload the Code**:
   - Open the `Grafico_BTC.ino` file in the Arduino IDE.
   - Enter your Wi-Fi credentials and any API details required to fetch Bitcoin data.
   - Connect the ESP32 to your computer, select the correct **Port** under **Tools**, and click **Upload**.

## Usage

Once the device is powered and connected to Wi-Fi:
- The ESP32 will fetch the latest Bitcoin price from the chosen API.
- The price will be displayed on the TFT screen, updating regularly to provide near-real-time price data.

## Additional Files

The project includes two bitmap files for display purposes:
- **background.h**: Contains the background image data for the display.
- **btc_png.h**: Contains a Bitcoin logo image data for display on the screen.

These files should be included in the same directory as the main `Grafico_BTC.ino` file to ensure they are accessible when the program is compiled and uploaded.

## License

This project is open-source. You are free to modify and distribute it under the terms of the MIT License.
Please refer the original creator "Giorgio Leggio" https://github.com/gio83dj in the modifications.

---
