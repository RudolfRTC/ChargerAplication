"""Telemetry panel: real-time Message2 display with status indicators."""

from __future__ import annotations

import time
from typing import Optional

from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QGroupBox,
    QGridLayout,
    QLabel,
    QVBoxLayout,
    QHBoxLayout,
    QFrame,
)

from obc_controller.can_protocol import Message2, StatusFlags


class _Indicator(QFrame):
    """Small coloured indicator: green = OK, red = FAULT."""

    def __init__(self, label_text: str, parent=None):
        super().__init__(parent)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        self._dot = QLabel("\u25CF")  # filled circle
        self._dot.setStyleSheet("color: green; font-size: 14px;")
        self._label = QLabel(label_text)
        layout.addWidget(self._dot)
        layout.addWidget(self._label)
        layout.addStretch()

    def set_ok(self, ok: bool) -> None:
        if ok:
            self._dot.setStyleSheet("color: green; font-size: 14px;")
        else:
            self._dot.setStyleSheet("color: red; font-size: 14px;")


class TelemetryPanel(QGroupBox):
    def __init__(self, parent=None):
        super().__init__("Telemetry (Message2)", parent)
        layout = QVBoxLayout(self)

        grid = QGridLayout()
        self._vout_label = QLabel("—")
        self._iout_label = QLabel("—")
        self._vin_label = QLabel("—")
        self._temp_label = QLabel("—")

        for lbl in (
            self._vout_label,
            self._iout_label,
            self._vin_label,
            self._temp_label,
        ):
            lbl.setStyleSheet("font-size: 16px; font-weight: bold;")

        grid.addWidget(QLabel("Output voltage:"), 0, 0)
        grid.addWidget(self._vout_label, 0, 1)
        grid.addWidget(QLabel("Output current:"), 1, 0)
        grid.addWidget(self._iout_label, 1, 1)
        grid.addWidget(QLabel("Input voltage:"), 2, 0)
        grid.addWidget(self._vin_label, 2, 1)
        grid.addWidget(QLabel("Temperature:"), 3, 0)
        grid.addWidget(self._temp_label, 3, 1)
        layout.addLayout(grid)

        # Status flags
        self._hw_ind = _Indicator("Hardware")
        self._temp_ind = _Indicator("Temperature")
        self._vin_ind = _Indicator("Input Voltage")
        self._start_ind = _Indicator("Starting State")
        self._comm_ind = _Indicator("Communication")

        flags_layout = QHBoxLayout()
        for ind in (
            self._hw_ind,
            self._temp_ind,
            self._vin_ind,
            self._start_ind,
            self._comm_ind,
        ):
            flags_layout.addWidget(ind)
        layout.addLayout(flags_layout)

        # Last received + alarm
        bottom = QHBoxLayout()
        self._last_rx_label = QLabel("Last received: —")
        self._alarm_label = QLabel("")
        self._alarm_label.setStyleSheet(
            "color: red; font-weight: bold; font-size: 13px;"
        )
        bottom.addWidget(self._last_rx_label)
        bottom.addStretch()
        bottom.addWidget(self._alarm_label)
        layout.addLayout(bottom)

    def update_telemetry(self, msg: Message2) -> None:
        self._vout_label.setText(f"{msg.output_voltage:.1f} V")
        self._iout_label.setText(f"{msg.output_current:.1f} A")
        self._vin_label.setText(f"{msg.input_voltage:.1f} V")
        self._temp_label.setText(f"{msg.temperature:.1f} \u00b0C")

        s = msg.status
        self._hw_ind.set_ok(not s.hardware_failure)
        self._temp_ind.set_ok(not s.over_temperature)
        self._vin_ind.set_ok(not s.input_voltage_error)
        self._start_ind.set_ok(not s.starting_state)
        self._comm_ind.set_ok(not s.communication_timeout)

        self._last_rx_label.setText(
            f"Last received: {time.strftime('%H:%M:%S')}"
        )
        self._alarm_label.setText("")

    def set_alarm(self, text: str) -> None:
        self._alarm_label.setText(text)

    def clear(self) -> None:
        for lbl in (
            self._vout_label,
            self._iout_label,
            self._vin_label,
            self._temp_label,
        ):
            lbl.setText("—")
        self._last_rx_label.setText("Last received: —")
        self._alarm_label.setText("")
