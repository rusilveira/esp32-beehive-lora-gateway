# ESP32 Beehive LoRa Gateway

ESP32-based LoRa gateway for receiving data from beehive sensor nodes.

## Overview

This project implements a LoRa gateway using ESP32 and SX1276.

The gateway is responsible for:

- receiving data packets from sensor nodes
- validating packets using CRC16
- sending ACK confirmations
- decoding payload data
- printing sensor data via serial (future: backend integration)

---

## Features

- LoRa communication (SX1276)
- Reliable protocol support (ACK, CRC, sequence)
- Duplicate packet detection
- Payload decoding
- Ready for backend integration

---

## Hardware

- ESP32
- SX1276 LoRa module

### Pin map

- SCK: GPIO 5
- MISO: GPIO 19
- MOSI: GPIO 27
- CS: GPIO 18
- DIO0: GPIO 26
- RST: GPIO 14

---

## Firmware Flow

1. Initialize LoRa radio
2. Wait for incoming packet
3. Validate packet (CRC + structure)
4. Send ACK
5. Check duplication
6. Decode payload
7. Print values

---

## Current Status

- Stable LoRa reception
- ACK working
- Duplicate detection working
- Payload decoding working

---

## Future Improvements

- HTTP API integration
- Database storage
- Web dashboard
- Multi-node support

---

## Author

Ruan Silveira