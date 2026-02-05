# OBC Charger Controller

Desktop application for controlling an OBC (On-Board Charger) module via CAN bus
(J1939, 29-bit extended frame) using a PCAN USB adapter.

## Features

- **CAN communication** — Message1 TX (BMS→OBC) and Message2 RX (OBC→BCA) per the
  CAN BUS COMMUNICATION SPECIFICATION v1.3
- **Real-time telemetry** — output voltage, current, input voltage, temperature,
  status flags with fault indicators
- **Live graphs** — Voltage (Vout/Vin) and Current/Temperature plots with
  configurable time window (1–30 min), pause, clear, CSV export
- **Control panel** — set max voltage/current, start/stop/heating modes
- **Safe disconnect** — sends Control=STOP for several cycles before closing the bus
- **Timeout alarm** — alarm if no Message2 received for > 5 s
- **Simulation mode** — test the UI without CAN hardware
- **Configurable bitrate** — 250 kbps or 500 kbps

## Requirements

- Python 3.10+
- PCAN USB adapter (with PCAN driver installed) **or** SocketCAN on Linux

## Installation

```bash
pip install -r requirements.txt
```

## Running

```bash
python main.py
```

## PCAN Bitrate Notes

The PCAN python-can backend may not support changing bitrate programmatically if
the channel is already initialized. In that case the app will log:

> Bitrate is configured in PCAN driver / PCAN-View

To change the bitrate:
1. Open **PCAN-View** (Windows) or **PCAN-Basic** configuration
2. Set the desired bitrate (250 kbps or 500 kbps)
3. Then connect from this app

## SocketCAN Setup (Linux)

```bash
# Bring up CAN interface at 250 kbps
sudo ip link set can0 type can bitrate 250000
sudo ip link set can0 up

# Or at 500 kbps
sudo ip link set can0 type can bitrate 500000
sudo ip link set can0 up

# Virtual CAN for testing
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set vcan0 up
```

Then select backend **socketcan** and channel **can0** (or **vcan0**) in the app.

## CAN Protocol Summary

| Message   | Direction  | ID           | Cycle  |
|-----------|-----------|--------------|--------|
| Message1  | BMS→OBC   | 0x1806E5F4   | 500 ms |
| Message2  | OBC→BCA   | 0x18FF50E5   | 500 ms |

Node addresses: BMS=0xF4, OBC=0xE5, BCA=0x50

### Message1 (8 bytes)
| Byte | Description                        | Encoding               |
|------|------------------------------------|------------------------|
| 0-1  | Max charging voltage               | 0.1 V/bit, big-endian  |
| 2-3  | Max charging current               | 0.1 A/bit, big-endian  |
| 4    | Control (0=start, 1=stop, 2=heat)  | unsigned               |
| 5-7  | Reserved                           | 0x00                   |

### Message2 (8 bytes)
| Byte | Description                        | Encoding               |
|------|------------------------------------|------------------------|
| 0-1  | Output voltage                     | 0.1 V/bit, big-endian  |
| 2-3  | Output current                     | 0.1 A/bit, big-endian  |
| 4    | Status flags (bit0–4)              | see below              |
| 5-6  | Input voltage                      | 0.1 V/bit, big-endian  |
| 7    | Temperature                        | byte − 40 = °C         |

Status flags (byte 4):
- bit0: Hardware failure
- bit1: Over-temperature
- bit2: Input voltage error
- bit3: Starting state (1 = stays off)
- bit4: Communication timeout

## Project Structure

```
main.py                          # Entry point
requirements.txt
obc_controller/
  can_protocol.py                # CAN codec — Message1/Message2 encode/decode
  can_worker.py                  # QThread CAN TX/RX worker
  simulator.py                   # Simulated Message2 generator
  ui/
    main_window.py               # Main window wiring
    connection_panel.py          # Connect/disconnect UI
    control_panel.py             # Voltage/current/mode controls
    telemetry_panel.py           # Real-time numeric display + status
    graph_panel.py               # pyqtgraph live plots
    log_panel.py                 # Scrollable log with save
```
