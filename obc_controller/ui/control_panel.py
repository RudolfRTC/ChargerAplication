"""Control panel: voltage/current setpoints, start/stop/heating buttons,
profile management, and ramp (soft-start) settings."""

from __future__ import annotations

import time

from PySide6.QtCore import Signal
from PySide6.QtWidgets import (
    QCheckBox,
    QComboBox,
    QDoubleSpinBox,
    QGroupBox,
    QHBoxLayout,
    QInputDialog,
    QLabel,
    QMessageBox,
    QPushButton,
    QVBoxLayout,
)

from obc_controller.can_protocol import ChargerControl
from obc_controller.profiles import (
    Profile,
    delete_profile,
    load_profiles,
    save_profile,
)


class ControlPanel(QGroupBox):
    # voltage, current, control, ramp_enabled, ramp_v, ramp_a
    control_changed = Signal(float, float, int, bool, float, float)
    instant_360v_requested = Signal()  # one-touch 360V/9A
    profile_loaded = Signal(object)  # Profile dataclass
    log_message = Signal(str)

    def __init__(self, parent=None):
        super().__init__("Control", parent)
        self._current_control = ChargerControl.STOP_OUTPUTTING

        layout = QVBoxLayout(self)

        # ---- Profiles section ----
        prof_group = QGroupBox("Profiles")
        prof_layout = QVBoxLayout(prof_group)

        prof_row1 = QHBoxLayout()
        prof_row1.addWidget(QLabel("Profile:"))
        self._profile_combo = QComboBox()
        self._profile_combo.setMinimumWidth(160)
        prof_row1.addWidget(self._profile_combo, stretch=1)
        prof_layout.addLayout(prof_row1)

        prof_row2 = QHBoxLayout()
        self._load_btn = QPushButton("Load")
        self._save_btn = QPushButton("Save")
        self._delete_btn = QPushButton("Delete")
        prof_row2.addWidget(self._load_btn)
        prof_row2.addWidget(self._save_btn)
        prof_row2.addWidget(self._delete_btn)
        prof_layout.addLayout(prof_row2)

        layout.addWidget(prof_group)

        # ---- Setpoints ----
        row_v = QHBoxLayout()
        row_v.addWidget(QLabel("Max charging voltage (V):"))
        self._voltage_spin = QDoubleSpinBox()
        self._voltage_spin.setRange(0.0, 6553.5)
        self._voltage_spin.setDecimals(1)
        self._voltage_spin.setSingleStep(0.1)
        self._voltage_spin.setValue(320.0)
        row_v.addWidget(self._voltage_spin)
        layout.addLayout(row_v)

        row_i = QHBoxLayout()
        row_i.addWidget(QLabel("Max charging current (A):"))
        self._current_spin = QDoubleSpinBox()
        self._current_spin.setRange(0.0, 6553.5)
        self._current_spin.setDecimals(1)
        self._current_spin.setSingleStep(0.1)
        self._current_spin.setValue(50.0)
        row_i.addWidget(self._current_spin)
        layout.addLayout(row_i)

        # ---- Mode buttons ----
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

        self._mode_label = QLabel("Mode: STOP")
        self._mode_label.setStyleSheet("font-weight: bold; font-size: 13px;")
        layout.addWidget(self._mode_label)

        # ---- Instant preset ----
        self._instant_btn = QPushButton("Set 360V / 9A (Instant)")
        self._instant_btn.setStyleSheet(
            "QPushButton { background-color: #9C27B0; color: white; "
            "padding: 8px 16px; font-weight: bold; font-size: 13px; }"
        )
        self._instant_btn.setToolTip(
            "One-touch: 360V / 9A, ramp OFF, Heating/DC Supply mode"
        )
        layout.addWidget(self._instant_btn)

        # ---- Ramp (soft-start) section ----
        ramp_group = QGroupBox("Ramp (soft-start)")
        ramp_layout = QVBoxLayout(ramp_group)

        self._ramp_check = QCheckBox("Enable Ramp")
        ramp_layout.addWidget(self._ramp_check)

        ramp_v_row = QHBoxLayout()
        ramp_v_row.addWidget(QLabel("Voltage ramp (V/s):"))
        self._ramp_v_spin = QDoubleSpinBox()
        self._ramp_v_spin.setRange(0.1, 500.0)
        self._ramp_v_spin.setDecimals(1)
        self._ramp_v_spin.setSingleStep(0.5)
        self._ramp_v_spin.setValue(5.0)
        ramp_v_row.addWidget(self._ramp_v_spin)
        ramp_layout.addLayout(ramp_v_row)

        ramp_a_row = QHBoxLayout()
        ramp_a_row.addWidget(QLabel("Current ramp (A/s):"))
        self._ramp_a_spin = QDoubleSpinBox()
        self._ramp_a_spin.setRange(0.1, 500.0)
        self._ramp_a_spin.setDecimals(1)
        self._ramp_a_spin.setSingleStep(0.1)
        self._ramp_a_spin.setValue(0.5)
        ramp_a_row.addWidget(self._ramp_a_spin)
        ramp_layout.addLayout(ramp_a_row)

        # Ramp status display
        ramp_status_row = QHBoxLayout()
        self._ramp_active_label = QLabel("")
        self._ramp_active_label.setStyleSheet("color: #2196F3; font-weight: bold;")
        ramp_status_row.addWidget(self._ramp_active_label)
        ramp_status_row.addStretch()
        ramp_layout.addLayout(ramp_status_row)

        ramp_vals_row = QHBoxLayout()
        self._ramp_v_actual = QLabel("Ramped V: —")
        self._ramp_a_actual = QLabel("Ramped A: —")
        ramp_vals_row.addWidget(self._ramp_v_actual)
        ramp_vals_row.addWidget(self._ramp_a_actual)
        ramp_vals_row.addStretch()
        ramp_layout.addLayout(ramp_vals_row)

        layout.addWidget(ramp_group)

        # ---- Wire signals ----
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
        self._ramp_check.toggled.connect(self._emit_state)
        self._ramp_v_spin.valueChanged.connect(self._emit_state)
        self._ramp_a_spin.valueChanged.connect(self._emit_state)

        self._instant_btn.clicked.connect(self._on_instant_360v)

        self._load_btn.clicked.connect(self._on_load_profile)
        self._save_btn.clicked.connect(self._on_save_profile)
        self._delete_btn.clicked.connect(self._on_delete_profile)

        self._refresh_profiles()
        self.setEnabled(False)

    # ---- Instant 360V / 9A preset ----

    def _on_instant_360v(self) -> None:
        self._voltage_spin.setValue(360.0)
        self._current_spin.setValue(9.0)
        self._ramp_check.setChecked(False)
        self._set_control(ChargerControl.HEATING_DC_SUPPLY)
        self._mode_label.setText("Mode: 360V / 9A INSTANT")
        self.instant_360v_requested.emit()
        self.log_message.emit(
            "Instant preset: 360V / 9A, ramp OFF, Heating/DC Supply mode"
        )

    # ---- Profile management ----

    def _refresh_profiles(self) -> None:
        current = self._profile_combo.currentText()
        self._profile_combo.clear()
        profiles = load_profiles()
        for name in sorted(profiles.keys()):
            self._profile_combo.addItem(name)
        idx = self._profile_combo.findText(current)
        if idx >= 0:
            self._profile_combo.setCurrentIndex(idx)

    def _on_save_profile(self) -> None:
        default_name = (
            self._profile_combo.currentText()
            or f"Profile {time.strftime('%H%M%S')}"
        )
        name, ok = QInputDialog.getText(
            self, "Save Profile", "Profile name:", text=default_name
        )
        if not ok or not name.strip():
            return
        name = name.strip()

        mode = "charging"
        if self._current_control == ChargerControl.HEATING_DC_SUPPLY:
            mode = "heating"

        profile = Profile(
            name=name,
            voltage_set_v=self._voltage_spin.value(),
            current_set_a=self._current_spin.value(),
            mode=mode,
            ramp_enabled=self._ramp_check.isChecked(),
            ramp_rate_v_per_s=self._ramp_v_spin.value(),
            ramp_rate_a_per_s=self._ramp_a_spin.value(),
        )
        save_profile(profile)
        self._refresh_profiles()
        self._profile_combo.setCurrentText(name)
        self.log_message.emit(f"Profile '{name}' saved.")

    def _on_load_profile(self) -> None:
        name = self._profile_combo.currentText()
        if not name:
            return
        profiles = load_profiles()
        p = profiles.get(name)
        if p is None:
            self.log_message.emit(f"Profile '{name}' not found.")
            return

        self._voltage_spin.setValue(p.voltage_set_v)
        self._current_spin.setValue(p.current_set_a)
        self._ramp_check.setChecked(p.ramp_enabled)
        self._ramp_v_spin.setValue(p.ramp_rate_v_per_s)
        self._ramp_a_spin.setValue(p.ramp_rate_a_per_s)

        if p.mode == "heating":
            self._set_control(ChargerControl.HEATING_DC_SUPPLY)
        else:
            self._set_control(ChargerControl.START_CHARGING)

        self.profile_loaded.emit(p)
        self.log_message.emit(
            f"Profile '{name}' loaded: {p.voltage_set_v:.1f}V / "
            f"{p.current_set_a:.1f}A / {p.mode} / "
            f"ramp={'ON' if p.ramp_enabled else 'OFF'}"
        )

    def _on_delete_profile(self) -> None:
        name = self._profile_combo.currentText()
        if not name:
            return
        reply = QMessageBox.question(
            self,
            "Delete Profile",
            f"Delete profile '{name}'?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
        )
        if reply == QMessageBox.StandardButton.Yes:
            delete_profile(name)
            self._refresh_profiles()
            self.log_message.emit(f"Profile '{name}' deleted.")

    # ---- Mode control ----

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
            self._ramp_check.isChecked(),
            self._ramp_v_spin.value(),
            self._ramp_a_spin.value(),
        )

    # ---- Ramp display (called from main window) ----

    def update_ramp_display(
        self, active: bool, ramped_v: float, ramped_a: float
    ) -> None:
        if active:
            self._ramp_active_label.setText("RAMP ACTIVE")
            self._ramp_v_actual.setText(f"Ramped V: {ramped_v:.1f}")
            self._ramp_a_actual.setText(f"Ramped A: {ramped_a:.1f}")
        else:
            self._ramp_active_label.setText("")
            self._ramp_v_actual.setText("Ramped V: —")
            self._ramp_a_actual.setText("Ramped A: —")

    # ---- Public getters ----

    def get_voltage(self) -> float:
        return self._voltage_spin.value()

    def get_current(self) -> float:
        return self._current_spin.value()

    def get_control(self) -> ChargerControl:
        return self._current_control

    def get_ramp_enabled(self) -> bool:
        return self._ramp_check.isChecked()

    def get_ramp_rates(self) -> tuple[float, float]:
        return self._ramp_v_spin.value(), self._ramp_a_spin.value()
