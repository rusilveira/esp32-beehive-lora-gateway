# ESP32 Beehive LoRa Gateway

ESP32-based LoRa gateway for receiving and decoding data from smart beehive sensor nodes.

---

## Overview

This project implements a **LoRa gateway using ESP32 + SX1276**, designed to receive data from remote beehive monitoring nodes.

The gateway is responsible for:

- receiving data packets from sensor nodes
- validating packets using CRC16
- sending ACK confirmations (reliable communication)
- detecting duplicate packets
- decoding structured payload data
- printing sensor data via serial output

---

## Features

- LoRa communication (SX1276 / RadioLib)
- Reliable protocol (ACK + CRC16 + sequence control)
- Duplicate packet detection
- Structured payload decoding (beehive data)
- Clean and deterministic firmware behavior
- Ready for backend integration (next step)

---

## Payload Structure

The gateway decodes the following data sent by the node:

- Internal temperature (°C)
- Internal humidity (%)
- External temperature (°C)
- External humidity (%)
- Hive weight (kg)
- Battery voltage (V)

All values are transmitted in scaled integer format for efficiency.

---

## Hardware

- ESP32
- SX1276 LoRa module

### Pin Mapping

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
3. Validate packet (CRC + protocol structure)
4. Send ACK to node
5. Check for duplicate packet (sequence control)
6. Decode payload data
7. Print formatted values via serial

---

## Example Output

RX node=1 seq=10

Peso (kg): 0.573
Bateria (V): 4.107
Temp interna (C): 22.30
Umid interna (%): 62.80
Temp externa (C): 22.10
Umid externa (%): 63.90

---

## Current Status

- Stable LoRa communication
- Reliable ACK protocol working
- CRC validation working
- Duplicate detection working
- Payload decoding fully operational

---

## Next Steps

- Integration with backend (HTTP / API)
- Data persistence (database)
- Web dashboard
- Multi-node scalability

---

## Related Projects

- Beehive sensor node (ESP32 + LoRa + sensors)
- Backend API (Node.js)
- Monitoring dashboard

---

## Author

Ruan Silveira