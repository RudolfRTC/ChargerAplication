"""Telemetry panel: real-time Message2 display with LED pill indicators."""

from __future__ import annotations

import time

from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QFrame,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QVBoxLayout,
)

from obc_controller.can_protocol import Message2, StatusFlags
from obc_controller.ui.theme import CYAN, GREEN, RED, TEXT_DIM


class _LedPill(QFrame):
    """LED pill indicator: green glow = OK, red glow = FAULT."""

    _OK_STYLE = (
        "QFrame#led_pill {{ background: rgba(0,230,118,0.1); "
        "border: 1px solid rgba(0,230,118,0.3); border-radius: 12px; }}"
    )
    _FAULT_STYLE = (
        "QFrame#led_pill {{ background: rgba(255,23,68,0.15); "
        "border: 1px solid rgba(255,23,68,0.4); border-radius: 12px; }}"
    )

    def __init__(self, label_text: str, parent=None):
        super().__init__(parent)
        self.setObjectName("led_pill")
        lay = QHBoxLayout(self)
        lay.setContentsMargins(8, 3, 10, 3)
        lay.setSpacing(6)
        self._led = QLabel("\u25cf")
        self._label = QLabel(label_text)
        self._label.setStyleSheet(f"font-size: 11px; color: {TEXT_DIM};")
        lay.addWidget(self._led)
        lay.addWidget(self._label)
        self.set_ok(True)

    def set_ok(self, ok: bool) -> None:
        if ok:
            self._led.setStyleSheet(
                f"color: {GREEN}; font-size: 16px; background: transparent;"
            )
            self.setStyleSheet(self._OK_STYLE)
        else:
            self._led.setStyleSheet(
                f"color: {RED}; font-size: 16px; background: transparent;"
            )
            self.setStyleSheet(self._FAULT_STYLE)


class TelemetryPanel(QGroupBox):
    def __init__(self, parent=None):
        super().__init__("Telemetry", parent)
        layout = QVBoxLayout(self)

        # Value grid
        grid = QGridLayout()
        grid.setSpacing(6)

        self._vout_label = QLabel("\u2014")
        self._iout_label = QLabel("\u2014")
        self._vin_label = QLabel("\u2014")
        self._temp_label = QLabel("\u2014")

        for lbl in (
            self._vout_label,
            self._iout_label,
            self._vin_label,
            self._temp_label,
        ):
            lbl.setObjectName("tele_value")

        labels = ["Output V:", "Output A:", "Input V:", "Temp:"]
        values = [
            self._vout_label,
            self._iout_label,
            self._vin_label,
            self._temp_label,
        ]
        for i, (txt, val) in enumerate(zip(labels, values)):
            name_lbl = QLabel(txt)
            name_lbl.setObjectName("tele_label")
            grid.addWidget(name_lbl, 0, i * 2)
            grid.addWidget(val, 0, i * 2 + 1)

        layout.addLayout(grid)

        # LED pill status flags
        self._hw_ind = _LedPill("HW")
        self._temp_ind = _LedPill("Temp")
        self._vin_ind = _LedPill("Vin")
        self._start_ind = _LedPill("Start")
        self._comm_ind = _LedPill("Comm")

        pill_row = QHBoxLayout()
        pill_row.setSpacing(6)
        for ind in (
            self._hw_ind,
            self._temp_ind,
            self._vin_ind,
            self._start_ind,
            self._comm_ind,
        ):
            pill_row.addWidget(ind)
        pill_row.addStretch()
        layout.addLayout(pill_row)

        # Last received + alarm
        bottom = QHBoxLayout()
        self._last_rx_label = QLabel("Last RX: \u2014")
        self._last_rx_label.setStyleSheet(f"color: {TEXT_DIM}; font-size: 11px;")
        self._alarm_label = QLabel("")
        self._alarm_label.setObjectName("alarm_label")
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
            f"Last RX: {time.strftime('%H:%M:%S')}"
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
            lbl.setText("\u2014")
        self._last_rx_label.setText("Last RX: \u2014")
        self._alarm_label.setText("")
