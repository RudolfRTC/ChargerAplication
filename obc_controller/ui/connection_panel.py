"""Connection panel: backend, channel, bitrate, connect/disconnect."""

from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import (
    QComboBox,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QPushButton,
    QVBoxLayout,
    QCheckBox,
)


class ConnectionPanel(QGroupBox):
    connect_requested = Signal(str, str, int, bool)  # iface, channel, bitrate, sim
    disconnect_requested = Signal()
    baudrate_switch_requested = Signal()

    def __init__(self, parent=None):
        super().__init__("Connection", parent)

        layout = QVBoxLayout(self)

        # Row 1: backend + channel
        row1 = QHBoxLayout()
        row1.addWidget(QLabel("Backend:"))
        self._backend_combo = QComboBox()
        self._backend_combo.addItems(["pcan", "socketcan"])
        row1.addWidget(self._backend_combo)

        row1.addWidget(QLabel("Channel:"))
        self._channel_edit = QLineEdit("PCAN_USBBUS1")
        self._channel_edit.setMinimumWidth(140)
        row1.addWidget(self._channel_edit)
        layout.addLayout(row1)

        # Row 2: bitrate + simulate
        row2 = QHBoxLayout()
        row2.addWidget(QLabel("CAN bitrate:"))
        self._bitrate_combo = QComboBox()
        self._bitrate_combo.addItems(["250000", "500000"])
        row2.addWidget(self._bitrate_combo)

        self._sim_check = QCheckBox("Simulate (no HW)")
        row2.addWidget(self._sim_check)
        row2.addStretch()
        layout.addLayout(row2)

        # Row 3: connect / disconnect + status
        row3 = QHBoxLayout()
        self._connect_btn = QPushButton("Connect")
        self._disconnect_btn = QPushButton("Disconnect")
        self._disconnect_btn.setEnabled(False)
        self._status_label = QLabel("Disconnected")
        self._status_label.setStyleSheet("color: gray; font-weight: bold;")

        row3.addWidget(self._connect_btn)
        row3.addWidget(self._disconnect_btn)
        row3.addWidget(self._status_label)
        row3.addStretch()
        layout.addLayout(row3)

        # Row 4: Service — baudrate switch
        row4 = QHBoxLayout()
        self._baud_switch_btn = QPushButton("Change Baudrate to 500k")
        self._baud_switch_btn.setStyleSheet(
            "QPushButton { background-color: #607D8B; color: white; "
            "padding: 6px 14px; font-weight: bold; }"
            "QPushButton:disabled { background-color: #B0BEC5; }"
        )
        self._baud_switch_btn.setToolTip(
            "Send CAN sequence to switch OBC device to 500 kbps"
        )
        self._baud_switch_btn.setEnabled(False)
        self._baud_status = QLabel("")
        self._baud_status.setStyleSheet("font-weight: bold;")
        row4.addWidget(self._baud_switch_btn)
        row4.addWidget(self._baud_status)
        row4.addStretch()
        layout.addLayout(row4)

        # Signals
        self._connect_btn.clicked.connect(self._on_connect)
        self._disconnect_btn.clicked.connect(self._on_disconnect)
        self._baud_switch_btn.clicked.connect(self._on_baud_switch)
        self._backend_combo.currentTextChanged.connect(self._on_backend_changed)

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

    def set_connected(self, connected: bool) -> None:
        self._connect_btn.setEnabled(not connected)
        self._disconnect_btn.setEnabled(connected)
        self._backend_combo.setEnabled(not connected)
        self._channel_edit.setEnabled(not connected)
        self._bitrate_combo.setEnabled(not connected)
        self._sim_check.setEnabled(not connected)
        self._baud_switch_btn.setEnabled(connected)
        if connected:
            self._status_label.setText("Connected")
            self._status_label.setStyleSheet(
                "color: green; font-weight: bold;"
            )
        else:
            self._status_label.setText("Disconnected")
            self._status_label.setStyleSheet(
                "color: gray; font-weight: bold;"
            )
            self._baud_status.setText("")

    def set_baud_switch_busy(self, busy: bool) -> None:
        """Disable/enable the button while sequence is running."""
        self._baud_switch_btn.setEnabled(not busy)

    def set_baud_switch_progress(self, step: int, total: int) -> None:
        self._baud_status.setText(
            f"Switching baudrate to 500k… ({step}/{total})"
        )
        self._baud_status.setStyleSheet(
            "color: #FF9800; font-weight: bold;"
        )

    def set_baud_switch_done(self) -> None:
        self._baud_status.setText("Baudrate switch done")
        self._baud_status.setStyleSheet(
            "color: green; font-weight: bold;"
        )
        self._baud_switch_btn.setEnabled(True)
