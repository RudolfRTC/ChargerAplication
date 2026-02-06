"""Control panel: voltage/current setpoints, start/stop/heating buttons,
profile management, ramp (soft-start), mode badge, and instant preset
with confirmation dialog."""

from __future__ import annotations

import time

from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (
    QCheckBox,
    QComboBox,
    QDoubleSpinBox,
    QFrame,
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
from obc_controller.settings import load_settings, save_setting
from obc_controller.ui.theme import CYAN, GREEN, ORANGE, RED, VIOLET


# Badge colour configs: (border/text colour, bg_start rgba, bg_end rgba)
_BADGE = {
    "stop": (RED, "rgba(255,23,68,0.25)", "rgba(255,23,68,0.08)"),
    "charging": (GREEN, "rgba(0,230,118,0.25)", "rgba(0,230,118,0.08)"),
    "heating": (ORANGE, "rgba(255,145,0,0.25)", "rgba(255,145,0,0.08)"),
    "instant": (VIOLET, "rgba(124,77,255,0.30)", "rgba(124,77,255,0.10)"),
}


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
        layout.setSpacing(8)

        # ---- Large MODE badge ----
        self._mode_badge = QFrame()
        self._mode_badge.setFixedHeight(46)
        badge_lay = QHBoxLayout(self._mode_badge)
        badge_lay.setContentsMargins(0, 0, 0, 0)
        self._mode_badge_label = QLabel("STOP")
        self._mode_badge_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        badge_lay.addWidget(self._mode_badge_label)
        layout.addWidget(self._mode_badge)
        self._update_badge("STOP", "stop")

        # ---- Profiles section ----
        prof_group = QGroupBox("Profiles")
        prof_layout = QVBoxLayout(prof_group)

        prof_row1 = QHBoxLayout()
        prof_row1.addWidget(QLabel("Profile:"))
        self._profile_combo = QComboBox()
        self._profile_combo.setMinimumWidth(120)
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
        sp_group = QGroupBox("Setpoints")
        sp_layout = QVBoxLayout(sp_group)

        row_v = QHBoxLayout()
        row_v.addWidget(QLabel("Voltage (V):"))
        self._voltage_spin = QDoubleSpinBox()
        self._voltage_spin.setRange(0.0, 6553.5)
        self._voltage_spin.setDecimals(1)
        self._voltage_spin.setSingleStep(0.1)
        self._voltage_spin.setValue(320.0)
        row_v.addWidget(self._voltage_spin, stretch=1)
        sp_layout.addLayout(row_v)

        row_i = QHBoxLayout()
        row_i.addWidget(QLabel("Current (A):"))
        self._current_spin = QDoubleSpinBox()
        self._current_spin.setRange(0.0, 6553.5)
        self._current_spin.setDecimals(1)
        self._current_spin.setSingleStep(0.1)
        self._current_spin.setValue(50.0)
        row_i.addWidget(self._current_spin, stretch=1)
        sp_layout.addLayout(row_i)

        layout.addWidget(sp_group)

        # ---- Mode buttons ----
        self._start_btn = QPushButton("Start Charging")
        self._start_btn.setObjectName("btn_start")
        self._stop_btn = QPushButton("Stop Outputting")
        self._stop_btn.setObjectName("btn_stop")
        self._heat_btn = QPushButton("Heating / DC")
        self._heat_btn.setObjectName("btn_heat")

        layout.addWidget(self._start_btn)
        layout.addWidget(self._stop_btn)
        layout.addWidget(self._heat_btn)

        # ---- Instant preset ----
        self._instant_btn = QPushButton("\u26a1 360V / 9A Instant")
        self._instant_btn.setObjectName("btn_instant")
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
        ramp_v_row.addWidget(QLabel("V/s:"))
        self._ramp_v_spin = QDoubleSpinBox()
        self._ramp_v_spin.setRange(0.1, 500.0)
        self._ramp_v_spin.setDecimals(1)
        self._ramp_v_spin.setSingleStep(0.5)
        self._ramp_v_spin.setValue(5.0)
        ramp_v_row.addWidget(self._ramp_v_spin, stretch=1)
        ramp_layout.addLayout(ramp_v_row)

        ramp_a_row = QHBoxLayout()
        ramp_a_row.addWidget(QLabel("A/s:"))
        self._ramp_a_spin = QDoubleSpinBox()
        self._ramp_a_spin.setRange(0.1, 500.0)
        self._ramp_a_spin.setDecimals(1)
        self._ramp_a_spin.setSingleStep(0.1)
        self._ramp_a_spin.setValue(0.5)
        ramp_a_row.addWidget(self._ramp_a_spin, stretch=1)
        ramp_layout.addLayout(ramp_a_row)

        # Ramp status
        self._ramp_active_label = QLabel("")
        self._ramp_active_label.setObjectName("ramp_active")
        ramp_layout.addWidget(self._ramp_active_label)

        ramp_vals = QHBoxLayout()
        self._ramp_v_actual = QLabel("V: \u2014")
        self._ramp_a_actual = QLabel("A: \u2014")
        ramp_vals.addWidget(self._ramp_v_actual)
        ramp_vals.addWidget(self._ramp_a_actual)
        ramp_layout.addLayout(ramp_vals)

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

    # ---- Mode badge ----

    def _update_badge(self, text: str, style_key: str) -> None:
        color, bg_start, bg_end = _BADGE.get(
            style_key, _BADGE["stop"]
        )
        self._mode_badge_label.setText(text)
        self._mode_badge.setStyleSheet(
            f"QFrame {{ background: qlineargradient(x1:0,y1:0,x2:0,y2:1, "
            f"stop:0 {bg_start}, stop:1 {bg_end}); "
            f"border: 2px solid {color}; border-radius: 10px; }}"
        )
        self._mode_badge_label.setStyleSheet(
            f"color: {color}; font-size: 18px; font-weight: bold; "
            f"letter-spacing: 3px; background: transparent;"
        )

    # ---- Instant 360V / 9A preset ----

    def _on_instant_360v(self) -> None:
        # Confirmation dialog (unless user opted out)
        settings = load_settings()
        if not settings.get("skip_instant_confirm", False):
            dlg = QMessageBox(self)
            dlg.setWindowTitle("Confirm Instant Preset")
            dlg.setText(
                "This will immediately set 360 V / 9 A "
                "in HEATING/DC mode.\nContinue?"
            )
            dlg.setStandardButtons(
                QMessageBox.StandardButton.Yes
                | QMessageBox.StandardButton.Cancel
            )
            dlg.setDefaultButton(QMessageBox.StandardButton.Cancel)
            cb = QCheckBox("Don\u2019t ask again")
            dlg.setCheckBox(cb)
            result = dlg.exec()
            if result != QMessageBox.StandardButton.Yes:
                return
            if cb.isChecked():
                save_setting("skip_instant_confirm", True)

        self._voltage_spin.setValue(360.0)
        self._current_spin.setValue(9.0)
        self._ramp_check.setChecked(False)
        self._set_control(ChargerControl.HEATING_DC_SUPPLY)
        # Override badge to violet "instant" style
        self._update_badge("\u26a1 360V / 9A", "instant")
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
        badge_map = {
            ChargerControl.STOP_OUTPUTTING: ("STOP", "stop"),
            ChargerControl.START_CHARGING: ("CHARGING", "charging"),
            ChargerControl.HEATING_DC_SUPPLY: ("HEATING / DC", "heating"),
        }
        text, style_key = badge_map.get(ctrl, ("?", "stop"))
        self._update_badge(text, style_key)
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
            self._ramp_active_label.setText("\u25b6 RAMP ACTIVE")
            self._ramp_v_actual.setText(f"V: {ramped_v:.1f}")
            self._ramp_a_actual.setText(f"A: {ramped_a:.1f}")
        else:
            self._ramp_active_label.setText("")
            self._ramp_v_actual.setText("V: \u2014")
            self._ramp_a_actual.setText("A: \u2014")

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
