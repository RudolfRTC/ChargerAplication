"""Connection panel: backend, channel, bitrate, connect/disconnect,
baudrate switch, and CAN health monitor."""

from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import (
    QCheckBox,
    QComboBox,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QPushButton,
    QVBoxLayout,
)

from obc_controller.ui.theme import GREEN, RED, TEXT_DIM


class ConnectionPanel(QGroupBox):
    connect_requested = Signal(str, str, int, bool)  # iface, channel, bitrate, sim
    disconnect_requested = Signal()
    baudrate_switch_requested = Signal()

    def __init__(self, parent=None):
        super().__init__("Connection", parent)

        layout = QVBoxLayout(self)
        layout.setSpacing(8)

        # Row 1: backend
        row1 = QHBoxLayout()
        row1.addWidget(QLabel("Backend:"))
        self._backend_combo = QComboBox()
        self._backend_combo.addItems(["pcan", "socketcan"])
        row1.addWidget(self._backend_combo, stretch=1)
        layout.addLayout(row1)

        # Row 2: channel
        row2 = QHBoxLayout()
        row2.addWidget(QLabel("Channel:"))
        self._channel_edit = QLineEdit("PCAN_USBBUS1")
        row2.addWidget(self._channel_edit, stretch=1)
        layout.addLayout(row2)

        # Row 3: bitrate
        row3 = QHBoxLayout()
        row3.addWidget(QLabel("CAN bitrate:"))
        self._bitrate_combo = QComboBox()
        self._bitrate_combo.addItems(["250000", "500000"])
        row3.addWidget(self._bitrate_combo, stretch=1)
        layout.addLayout(row3)

        # Row 4: simulate checkbox
        self._sim_check = QCheckBox("Simulate (no HW)")
        layout.addWidget(self._sim_check)

        # Row 5: connect / disconnect
        btn_row = QHBoxLayout()
        self._connect_btn = QPushButton("Connect")
        self._connect_btn.setObjectName("btn_connect")
        self._disconnect_btn = QPushButton("Disconnect")
        self._disconnect_btn.setObjectName("btn_disconnect")
        self._disconnect_btn.setEnabled(False)
        btn_row.addWidget(self._connect_btn)
        btn_row.addWidget(self._disconnect_btn)
        layout.addLayout(btn_row)

        # Status label
        self._status_label = QLabel("Disconnected")
        self._status_label.setObjectName("status_disconnected")
        layout.addWidget(self._status_label)

        # Baudrate switch section
        self._baud_switch_btn = QPushButton("Baudrate \u2192 500k")
        self._baud_switch_btn.setObjectName("btn_baud")
        self._baud_switch_btn.setToolTip(
            "Send CAN sequence to switch OBC device to 500 kbps"
        )
        self._baud_switch_btn.setEnabled(False)
        layout.addWidget(self._baud_switch_btn)

        self._baud_status = QLabel("")
        self._baud_status.setObjectName("baud_status")
        layout.addWidget(self._baud_status)

        # ---- Health panel ----
        health_group = QGroupBox("Health")
        health_lay = QVBoxLayout(health_group)
        health_lay.setSpacing(4)

        _mono = (
            "font-family: 'Cascadia Code','Fira Code','Consolas',monospace; "
            f"font-size: 12px; color: {TEXT_DIM};"
        )
        self._tx_rate_lbl = QLabel("TX: \u2014 /s")
        self._rx_rate_lbl = QLabel("RX: \u2014 /s")
        self._rx_age_lbl = QLabel("Last RX: \u2014 s")
        self._comm_lbl = QLabel("Comm: \u2014")
        self._bitrate_lbl = QLabel("Bitrate: \u2014")

        for lbl in (
            self._tx_rate_lbl,
            self._rx_rate_lbl,
            self._rx_age_lbl,
            self._comm_lbl,
            self._bitrate_lbl,
        ):
            lbl.setStyleSheet(_mono)
            health_lay.addWidget(lbl)

        layout.addWidget(health_group)

        # ---- Signals ----
        self._connect_btn.clicked.connect(self._on_connect)
        self._disconnect_btn.clicked.connect(self._on_disconnect)
        self._baud_switch_btn.clicked.connect(self._on_baud_switch)
        self._backend_combo.currentTextChanged.connect(
            self._on_backend_changed
        )

    def _on_backend_changed(self, text: str) -> None:
        if text == "pcan":
            self._channel_edit.setText("PCAN_USBBUS1")
        elif text == "socketcan":
            self._channel_edit.setText("can0")

    def _on_connect(self) -> None:
        iface = self._backend_combo.currentText()
        channel = self._channel_edit.text().strip()
        bitrate = int(self._bitrate_combo.currentText())
        sim = self._sim_check.isChecked()
        self.connect_requested.emit(iface, channel, bitrate, sim)

    def _on_disconnect(self) -> None:
        self.disconnect_requested.emit()

    def _on_baud_switch(self) -> None:
        self.baudrate_switch_requested.emit()

    # ---- public state setters ----

    def set_connected(self, connected: bool) -> None:
        self._connect_btn.setEnabled(not connected)
        self._disconnect_btn.setEnabled(connected)
        self._backend_combo.setEnabled(not connected)
        self._channel_edit.setEnabled(not connected)
        self._bitrate_combo.setEnabled(not connected)
        self._sim_check.setEnabled(not connected)
        self._baud_switch_btn.setEnabled(connected)
        if connected:
            self._status_label.setText("\u25cf  Connected")
            self._status_label.setObjectName("status_connected")
            bitrate = int(self._bitrate_combo.currentText())
            self._bitrate_lbl.setText(f"Bitrate: {bitrate // 1000}k")
        else:
            self._status_label.setText("\u25cb  Disconnected")
            self._status_label.setObjectName("status_disconnected")
            self._reset_health()
        # Force style recalculation after objectName change
        self._status_label.style().unpolish(self._status_label)
        self._status_label.style().polish(self._status_label)
        self._baud_status.setText("")

    def update_health(
        self, tx_rate: float, rx_rate: float, last_rx_age: float
    ) -> None:
        """Update the health panel with live CAN bus stats."""
        self._tx_rate_lbl.setText(f"TX: {tx_rate:.1f} /s")
        self._rx_rate_lbl.setText(f"RX: {rx_rate:.1f} /s")
        self._rx_age_lbl.setText(f"Last RX: {last_rx_age:.1f} s")
        if last_rx_age > 5.0:
            self._comm_lbl.setText("Comm: TIMEOUT")
            self._comm_lbl.setStyleSheet(
                f"color: {RED}; font-weight: bold; "
                "font-family: 'Cascadia Code','Fira Code','Consolas',monospace; "
                "font-size: 12px;"
            )
        else:
            self._comm_lbl.setText("Comm: OK")
            self._comm_lbl.setStyleSheet(
                f"color: {GREEN}; font-weight: bold; "
                "font-family: 'Cascadia Code','Fira Code','Consolas',monospace; "
                "font-size: 12px;"
            )

    def _reset_health(self) -> None:
        _mono = (
            "font-family: 'Cascadia Code','Fira Code','Consolas',monospace; "
            f"font-size: 12px; color: {TEXT_DIM};"
        )
        self._tx_rate_lbl.setText("TX: \u2014 /s")
        self._rx_rate_lbl.setText("RX: \u2014 /s")
        self._rx_age_lbl.setText("Last RX: \u2014 s")
        self._comm_lbl.setText("Comm: \u2014")
        self._comm_lbl.setStyleSheet(_mono)
        self._bitrate_lbl.setText("Bitrate: \u2014")

    def set_baud_switch_busy(self, busy: bool) -> None:
        self._baud_switch_btn.setEnabled(not busy)

    def set_baud_switch_progress(self, step: int, total: int) -> None:
        self._baud_status.setText(
            f"Switching\u2026 ({step}/{total})"
        )
        self._baud_status.setStyleSheet(
            "color: #FF9800; font-weight: bold;"
        )

    def set_baud_switch_done(self) -> None:
        self._baud_status.setText("\u2713 Baudrate switch done")
        self._baud_status.setStyleSheet(
            "color: #00e676; font-weight: bold;"
        )
        self._baud_switch_btn.setEnabled(True)
