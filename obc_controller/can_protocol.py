"""
CAN protocol codec for OBC charger communication.

Based on CAN BUS COMMUNICATION SPECIFICATION v1.3:
  - J1939, CAN2.0B, 29-bit extended frame
  - Default bus speed: 250 kbps
  - Message1: BMS -> OBC  (ID 0x1806E5F4, 500 ms)
  - Message2: OBC -> BCA  (ID 0x18FF50E5, 500 ms)
"""

from __future__ import annotations

import struct
from dataclasses import dataclass, field
from enum import IntEnum
from typing import Optional


# ---------------------------------------------------------------------------
# Node source addresses (J1939)
# ---------------------------------------------------------------------------
SA_BMS = 0xF4   # 244
SA_OBC = 0xE5   # 229
SA_BCA = 0x50   # 80

# ---------------------------------------------------------------------------
# CAN message IDs (29-bit extended)
# ---------------------------------------------------------------------------
MSG1_ID = 0x1806E5F4   # BMS -> OBC
MSG2_ID = 0x18FF50E5   # OBC -> BCA

# ---------------------------------------------------------------------------
# Timing
# ---------------------------------------------------------------------------
CYCLE_MS = 500          # Transmit period (ms)
TIMEOUT_S = 5.0         # Communication timeout (s)


class ChargerControl(IntEnum):
    START_CHARGING = 0
    STOP_OUTPUTTING = 1
    HEATING_DC_SUPPLY = 2


# ---------------------------------------------------------------------------
# Message1  (BMS -> OBC)
# ---------------------------------------------------------------------------
@dataclass
class Message1:
    """Command message sent from BMS (this app) to OBC."""
    voltage_setpoint: float = 0.0   # V, resolution 0.1 V
    current_setpoint: float = 0.0   # A, resolution 0.1 A
    control: ChargerControl = ChargerControl.STOP_OUTPUTTING

    def encode(self) -> bytes:
        """Encode to 8-byte CAN payload (big-endian, unsigned)."""
        v_raw = int(round(self.voltage_setpoint * 10))
        i_raw = int(round(self.current_setpoint * 10))
        v_raw = max(0, min(v_raw, 0xFFFF))
        i_raw = max(0, min(i_raw, 0xFFFF))
        return struct.pack(">HHB3x", v_raw, i_raw, int(self.control))

    @classmethod
    def decode(cls, data: bytes) -> "Message1":
        """Decode 8-byte CAN payload."""
        if len(data) < 5:
            raise ValueError(f"Message1 requires >= 5 bytes, got {len(data)}")
        v_raw, i_raw, ctrl = struct.unpack_from(">HHB", data, 0)
        try:
            control = ChargerControl(ctrl)
        except ValueError:
            raise ValueError(
                f"Unknown ChargerControl value: {ctrl} "
                f"(expected 0, 1, or 2)"
            )
        return cls(
            voltage_setpoint=v_raw / 10.0,
            current_setpoint=i_raw / 10.0,
            control=control,
        )


# ---------------------------------------------------------------------------
# Status flags in Message2 BYTE4
# ---------------------------------------------------------------------------
@dataclass
class StatusFlags:
    hardware_failure: bool = False       # bit0: 0=Normal, 1=Failure
    over_temperature: bool = False       # bit1: 0=Normal, 1=Over-temp
    input_voltage_error: bool = False    # bit2: 0=Normal, 1=Wrong
    starting_state: bool = False         # bit3: 0=Charging, 1=Stays off
    communication_timeout: bool = False  # bit4: 0=Normal, 1=Timeout

    @classmethod
    def from_byte(cls, b: int) -> "StatusFlags":
        return cls(
            hardware_failure=bool(b & 0x01),
            over_temperature=bool(b & 0x02),
            input_voltage_error=bool(b & 0x04),
            starting_state=bool(b & 0x08),
            communication_timeout=bool(b & 0x10),
        )

    def to_byte(self) -> int:
        b = 0
        if self.hardware_failure:
            b |= 0x01
        if self.over_temperature:
            b |= 0x02
        if self.input_voltage_error:
            b |= 0x04
        if self.starting_state:
            b |= 0x08
        if self.communication_timeout:
            b |= 0x10
        return b

    def any_fault(self) -> bool:
        return (
            self.hardware_failure
            or self.over_temperature
            or self.input_voltage_error
            or self.communication_timeout
        )


# ---------------------------------------------------------------------------
# Message2  (OBC -> BCA)
# ---------------------------------------------------------------------------
@dataclass
class Message2:
    """Telemetry message broadcast by OBC."""
    output_voltage: float = 0.0    # V
    output_current: float = 0.0    # A
    status: StatusFlags = field(default_factory=StatusFlags)
    input_voltage: float = 0.0     # V
    temperature: float = 0.0       # degC

    def encode(self) -> bytes:
        """Encode to 8-byte CAN payload."""
        v_raw = int(round(self.output_voltage * 10))
        i_raw = int(round(self.output_current * 10))
        vin_raw = int(round(self.input_voltage * 10))
        temp_raw = int(round(self.temperature + 40))
        v_raw = max(0, min(v_raw, 0xFFFF))
        i_raw = max(0, min(i_raw, 0xFFFF))
        vin_raw = max(0, min(vin_raw, 0xFFFF))
        temp_raw = max(0, min(temp_raw, 0xFF))
        return struct.pack(
            ">HHBHB",
            v_raw, i_raw, self.status.to_byte(), vin_raw, temp_raw,
        )

    @classmethod
    def decode(cls, data: bytes) -> "Message2":
        """Decode 8-byte CAN payload."""
        if len(data) < 8:
            raise ValueError(f"Message2 requires 8 bytes, got {len(data)}")
        v_raw, i_raw, status_byte, vin_raw, temp_raw = struct.unpack_from(
            ">HHBHB", data, 0
        )
        return cls(
            output_voltage=v_raw / 10.0,
            output_current=i_raw / 10.0,
            status=StatusFlags.from_byte(status_byte),
            input_voltage=vin_raw / 10.0,
            temperature=temp_raw - 40,
        )
