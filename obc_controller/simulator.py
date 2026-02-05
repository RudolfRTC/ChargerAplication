"""
Simulator that generates fake Message2 frames at 500 ms intervals.

Used when no CAN hardware is present (checkbox "Simulate").
"""

from __future__ import annotations

import math
import random
import time

from PySide6.QtCore import QThread, Signal

from obc_controller.can_protocol import (
    CYCLE_MS,
    Message2,
    StatusFlags,
)


class Simulator(QThread):
    """Generates synthetic Message2 data for UI testing."""

    message2_received = Signal(object)
    log_message = Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._running = False

    def request_stop(self) -> None:
        self._running = False

    def run(self) -> None:
        self._running = True
        self.log_message.emit("Simulator started (500 ms cycle).")
        t0 = time.monotonic()

        while self._running:
            elapsed = time.monotonic() - t0

            # Generate slowly varying values
            vout = 320.0 + 10.0 * math.sin(elapsed / 30.0)
            iout = 50.0 + 5.0 * math.sin(elapsed / 20.0 + 1.0)
            vin = 220.0 + 3.0 * math.sin(elapsed / 60.0)
            temp = 45.0 + 10.0 * math.sin(elapsed / 40.0 + 2.0)

            # Occasionally flip a status bit for realism
            hw_fail = random.random() < 0.005
            status = StatusFlags(hardware_failure=hw_fail)

            msg2 = Message2(
                output_voltage=round(vout, 1),
                output_current=round(iout, 1),
                status=status,
                input_voltage=round(vin, 1),
                temperature=round(temp, 1),
            )
            self.message2_received.emit(msg2)

            time.sleep(CYCLE_MS / 1000.0)

        self.log_message.emit("Simulator stopped.")
