# ESP32 Beehive LoRa Gateway

ESP32-based LoRa gateway responsible for receiving, validating, decoding and forwarding data from smart beehive sensor nodes.

> Part of the **[Smart Beehive Monitoring System](https://github.com/rusilveira/rusilveira#smart-beehive-monitoring-system)**: [sensor node](https://github.com/rusilveira/esp32-beehive-sensor-node-lora) · [LoRa link](https://github.com/rusilveira/esp32-lora-reliable-link) · **gateway** · [API](https://github.com/rusilveira/mms-backend) · [dashboard](https://github.com/rusilveira/mms-frontend)

---

## Problem

Sensor nodes in the field cannot reach the internet directly, and a radio link can lose, corrupt or repeat packets.

## Solution

An ESP32 gateway that receives LoRa packets, checks their integrity, confirms delivery to the node, discards duplicates and forwards each reading to the API over Wi-Fi. It supervises Wi-Fi and API health and restarts itself after persistent failures.

---

## Overview

This project implements a **LoRa + Wi-Fi gateway using ESP32 and SX1276**, acting as the bridge between remote sensor nodes and the cloud backend.

The gateway receives environmental data from beehive nodes via LoRa and forwards it to a backend API over Wi-Fi, enabling real-time monitoring and data persistence.

---

## Features

* LoRa communication (SX1276 / RadioLib)
* Reliable protocol (ACK + CRC16 + sequence control)
* Duplicate packet detection
* Structured payload decoding
* HTTP integration with backend API
* Automatic Wi-Fi reconnection
* Failure detection and recovery mechanisms
* Safe restart on persistent failures
* Designed for continuous field operation

---

## Payload Structure

The gateway decodes the following data sent by the node:

* Internal temperature (°C)
* Internal humidity (%)
* External temperature (°C)
* External humidity (%)
* Hive weight (kg)
* Battery voltage (V)

All values are transmitted in scaled integer format for efficiency and reliability.

---

## Hardware

* ESP32
* SX1276 LoRa module

### Pin Mapping

* SCK: GPIO 5
* MISO: GPIO 19
* MOSI: GPIO 27
* CS: GPIO 18
* DIO0: GPIO 26
* RST: GPIO 14

---

## Firmware Flow

1. Initialize Wi-Fi connection
2. Initialize LoRa radio
3. Wait for incoming packet
4. Validate packet (CRC + protocol structure)
5. Send ACK to node
6. Check for duplicate packet (sequence control)
7. Decode payload data
8. Forward data to backend API (HTTP POST)
9. Monitor system health (Wi-Fi + API + uptime)

---

## Connectivity & Reliability

The gateway includes mechanisms to ensure robust operation in real-world conditions:

### Wi-Fi Supervision

* periodic connection checks
* automatic reconnection when disconnected
* retry logic with timeout

### API Communication

* HTTP timeout handling
* failure counting
* retry strategy

### Fault Recovery

* restart triggered after:

  * multiple consecutive Wi-Fi failures
  * multiple consecutive API failures
  * extended offline period

### Offline Detection

* system monitors last successful communication
* triggers recovery if no successful transmission occurs for a defined period

---

## Example Output

```
RX node=1 seq=10

Peso (kg): 0.573
Bateria (V): 4.107
Temp interna (C): 22.30
Umid interna (%): 62.80
Temp externa (C): 22.10
Umid externa (%): 63.90

[API] HTTP code: 201
```

---

## Configuration

Wi-Fi credentials and backend API endpoint are defined in:

```
config.h
```

Example:

```cpp
#define WIFI_SSID "your_ssid"
#define WIFI_PASSWORD "your_password"
#define API_URL "http://your-backend-url/api/readings"
```

Keep real credentials only in your local copy. Never commit them.

---

## Build and flash

Requires [PlatformIO](https://platformio.org/) (VS Code extension or CLI).

Fill in `include/config.h` (see Configuration) before building.

```bash
git clone https://github.com/rusilveira/esp32-beehive-lora-gateway.git
cd esp32-beehive-lora-gateway
pio run --target upload
pio device monitor
```

---

## Current Status

* Stable LoRa communication
* Reliable ACK protocol
* CRC validation working
* Duplicate detection working
* Payload decoding fully operational
* Backend integration completed
* PostgreSQL persistence enabled via API
* Gateway recovery mechanisms implemented

---

## Next Steps

* OTA firmware update support
* Remote diagnostics
* multi-gateway support
* signal quality monitoring (RSSI/SNR analytics)
* energy optimization for long-term deployment

---

## System Architecture

```
ESP32 Node → LoRa → Gateway → Wi-Fi → Backend API → PostgreSQL → Dashboard
```

---

## Author

**Ruan Silveira** · Industrial instrumentation technician and Electrical Engineering student  
[GitHub](https://github.com/rusilveira) · [LinkedIn](https://www.linkedin.com/in/ruan-silveira-494110257)

## License

MIT. See [LICENSE](LICENSE).
