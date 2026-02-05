"""Control panel: voltage/current setpoints, start/stop/heating buttons."""

from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import (
    QDoubleSpinBox,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QVBoxLayout,
)

from obc_controller.can_protocol import ChargerControl


class ControlPanel(QGroupBox):
    control_changed = Signal(float, float, int)  # voltage, current, control

    def __init__(self, parent=None):
        super().__init__("Control", parent)
        self._current_control = ChargerControl.STOP_OUTPUTTING

        layout = QVBoxLayout(self)

        # Voltage setpoint
        row_v = QHBoxLayout()
        row_v.addWidget(QLabel("Max charging voltage (V):"))
        self._voltage_spin = QDoubleSpinBox()
        self._voltage_spin.setRange(0.0, 6553.5)
        self._voltage_spin.setDecimals(1)
        self._voltage_spin.setSingleStep(0.1)
        self._voltage_spin.setValue(320.0)
        row_v.addWidget(self._voltage_spin)
        layout.addLayout(row_v)

        # Current setpoint
        row_i = QHBoxLayout()
        row_i.addWidget(QLabel("Max charging current (A):"))
        self._current_spin = QDoubleSpinBox()
        self._current_spin.setRange(0.0, 6553.5)
        self._current_spin.setDecimals(1)
        self._current_spin.setSingleStep(0.1)
        self._current_spin.setValue(50.0)
        row_i.addWidget(self._current_spin)
        layout.addLayout(row_i)

        # Buttons
        btn_row = QHBoxLayout()
        self._start_btn = QPushButton("Start Charging")
        self._start_btn.setStyleSheet(
            "QPushButton { background-color: #4CAF50; color: white; "
            "padding: 6px 16px; font-weight: bold; }"
        )
        self._stop_btn = QPushButton("Stop Outputting")
        self._stop_btn.setStyleSheet(
            "QPushButton { background-color: #f44336; color: white; "
            "padding: 6px 16px; font-weight: bold; }"
        )
        self._heat_btn = QPushButton("Heating / DC Supply")
        self._heat_btn.setStyleSheet(
            "QPushButton { background-color: #FF9800; color: white; "
            "padding: 6px 16px; font-weight: bold; }"
        )

        btn_row.addWidget(self._start_btn)
        btn_row.addWidget(self._stop_btn)
        btn_row.addWidget(self._heat_btn)
        layout.addLayout(btn_row)

        # Active mode label
        self._mode_label = QLabel("Mode: STOP")
        self._mode_label.setStyleSheet("font-weight: bold; font-size: 13px;")
        layout.addWidget(self._mode_label)

        # Connections
        self._start_btn.clicked.connect(
            lambda: self._set_control(ChargerControl.START_CHARGING)
        )
        self._stop_btn.clicked.connect(
            lambda: self._set_control(ChargerControl.STOP_OUTPUTTING)
        )
        self._heat_btn.clicked.connect(
            lambda: self._set_control(ChargerControl.HEATING_DC_SUPPLY)
        )
        self._voltage_spin.valueChanged.connect(self._emit_state)
        self._current_spin.valueChanged.connect(self._emit_state)

        self.setEnabled(False)

    def _set_control(self, ctrl: ChargerControl) -> None:
        self._current_control = ctrl
        labels = {
            ChargerControl.START_CHARGING: "Mode: CHARGING",
            ChargerControl.STOP_OUTPUTTING: "Mode: STOP",
            ChargerControl.HEATING_DC_SUPPLY: "Mode: HEATING / DC SUPPLY",
        }
        self._mode_label.setText(labels.get(ctrl, "Mode: ?"))
        self._emit_state()

    def _emit_state(self) -> None:
        self.control_changed.emit(
            self._voltage_spin.value(),
            self._current_spin.value(),
            int(self._current_control),
        )

    def get_voltage(self) -> float:
        return self._voltage_spin.value()

    def get_current(self) -> float:
        return self._current_spin.value()

    def get_control(self) -> ChargerControl:
        return self._current_control
