"""Real-time graph panel using pyqtgraph.

Two sub-plots:
  1. Voltage: Vout and Vin (V)
  2. Current & Temperature: Iout (A, left axis) and Temperature (C, right axis)

Data is appended from the CAN thread; a Qt timer redraws at ~15 Hz so the UI
stays responsive regardless of CAN message rate.
"""

from __future__ import annotations

import csv
import time
from collections import deque
from pathlib import Path

import numpy as np
import pyqtgraph as pg
from PySide6.QtCore import QTimer
from PySide6.QtWidgets import (
    QComboBox,
    QFileDialog,
    QGroupBox,
    QHBoxLayout,
    QPushButton,
    QVBoxLayout,
    QLabel,
)

from obc_controller.can_protocol import Message2

# Time-window options (minutes)
WINDOW_OPTIONS = {
    "1 min": 60,
    "5 min": 300,
    "10 min": 600,
    "30 min": 1800,
}


class GraphPanel(QGroupBox):
    def __init__(self, parent=None):
        super().__init__("Live Graph", parent)

        self._t0 = time.monotonic()
        self._paused = False

        # Ring buffers (store up to 30 min @ 2 Hz = 3600 points max)
        maxlen = 3600
        self._ts = deque(maxlen=maxlen)
        self._vout = deque(maxlen=maxlen)
        self._vin = deque(maxlen=maxlen)
        self._iout = deque(maxlen=maxlen)
        self._temp = deque(maxlen=maxlen)
        self._status = deque(maxlen=maxlen)

        self._window_sec = 600  # default 10 min

        layout = QVBoxLayout(self)

        # --- controls ---
        ctrl_row = QHBoxLayout()
        self._pause_btn = QPushButton("Pause")
        self._pause_btn.setCheckable(True)
        self._pause_btn.toggled.connect(self._on_pause_toggled)
        ctrl_row.addWidget(self._pause_btn)

        self._clear_btn = QPushButton("Clear")
        self._clear_btn.clicked.connect(self._clear_data)
        ctrl_row.addWidget(self._clear_btn)

        ctrl_row.addWidget(QLabel("Window:"))
        self._window_combo = QComboBox()
        self._window_combo.addItems(list(WINDOW_OPTIONS.keys()))
        self._window_combo.setCurrentText("10 min")
        self._window_combo.currentTextChanged.connect(self._on_window_changed)
        ctrl_row.addWidget(self._window_combo)

        self._export_btn = QPushButton("Export CSV")
        self._export_btn.clicked.connect(self._export_csv)
        ctrl_row.addWidget(self._export_btn)

        ctrl_row.addStretch()
        layout.addLayout(ctrl_row)

        # --- pyqtgraph widget ---
        pg.setConfigOptions(antialias=True)
        self._graphics = pg.GraphicsLayoutWidget()
        layout.addWidget(self._graphics)

        # Plot 1: Voltage
        self._p1 = self._graphics.addPlot(row=0, col=0, title="Voltage")
        self._p1.setLabel("left", "Voltage", units="V")
        self._p1.setLabel("bottom", "Time", units="s")
        self._p1.addLegend()
        self._p1.showGrid(x=True, y=True, alpha=0.3)
        self._curve_vout = self._p1.plot(
            pen=pg.mkPen("y", width=2), name="Vout"
        )
        self._curve_vin = self._p1.plot(
            pen=pg.mkPen("c", width=2), name="Vin"
        )

        # Plot 2: Current + Temperature
        self._p2 = self._graphics.addPlot(row=1, col=0, title="Current & Temp")
        self._p2.setLabel("left", "Current", units="A")
        self._p2.setLabel("bottom", "Time", units="s")
        self._p2.addLegend()
        self._p2.showGrid(x=True, y=True, alpha=0.3)
        self._curve_iout = self._p2.plot(
            pen=pg.mkPen("#4CAF50", width=2), name="Iout"
        )

        # Second Y axis for temperature
        self._p2_temp = pg.ViewBox()
        self._p2.scene().addItem(self._p2_temp)
        self._p2.getAxis("right").linkToView(self._p2_temp)
        self._p2_temp.setXLink(self._p2)
        self._p2.setLabel("right", "Temperature", units="\u00b0C")
        self._p2.getAxis("right").show()
        self._curve_temp = pg.PlotCurveItem(
            pen=pg.mkPen("r", width=2), name="Temp"
        )
        self._p2_temp.addItem(self._curve_temp)
        # Keep temp ViewBox synced
        self._p2.getViewBox().sigResized.connect(self._sync_temp_viewbox)

        # Redraw timer (~15 Hz)
        self._redraw_timer = QTimer(self)
        self._redraw_timer.timeout.connect(self._redraw)
        self._redraw_timer.start(66)  # ~15 fps

    # ---- public API (called from main window) ----------------------------

    def add_point(self, msg: Message2) -> None:
        """Append a data point from Message2 (called from any thread via signal)."""
        t = time.monotonic() - self._t0
        self._ts.append(t)
        self._vout.append(msg.output_voltage)
        self._vin.append(msg.input_voltage)
        self._iout.append(msg.output_current)
        self._temp.append(msg.temperature)
        self._status.append(msg.status.to_byte())

    # ---- internal --------------------------------------------------------

    def _redraw(self) -> None:
        if self._paused or len(self._ts) == 0:
            return

        ts = np.array(self._ts)
        now = ts[-1]
        mask = ts >= (now - self._window_sec)

        t = ts[mask]
        self._curve_vout.setData(t, np.array(self._vout)[mask])
        self._curve_vin.setData(t, np.array(self._vin)[mask])
        self._curve_iout.setData(t, np.array(self._iout)[mask])
        self._curve_temp.setData(t, np.array(self._temp)[mask])

    def _sync_temp_viewbox(self) -> None:
        self._p2_temp.setGeometry(self._p2.getViewBox().sceneBoundingRect())
        self._p2_temp.linkedViewChanged(self._p2.getViewBox(), self._p2_temp.XAxis)

    def _on_pause_toggled(self, checked: bool) -> None:
        self._paused = checked
        self._pause_btn.setText("Resume" if checked else "Pause")

    def _on_window_changed(self, text: str) -> None:
        self._window_sec = WINDOW_OPTIONS.get(text, 600)

    def _clear_data(self) -> None:
        self._ts.clear()
        self._vout.clear()
        self._vin.clear()
        self._iout.clear()
        self._temp.clear()
        self._status.clear()
        self._t0 = time.monotonic()
        self._curve_vout.setData([], [])
        self._curve_vin.setData([], [])
        self._curve_iout.setData([], [])
        self._curve_temp.setData([], [])

    def _export_csv(self) -> None:
        path, _ = QFileDialog.getSaveFileName(
            self,
            "Export CSV",
            f"obc_data_{time.strftime('%Y%m%d_%H%M%S')}.csv",
            "CSV files (*.csv);;All files (*)",
        )
        if not path:
            return
        with open(path, "w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(
                ["timestamp_s", "Vout_V", "Iout_A", "Vin_V", "Temp_C", "status_flags"]
            )
            for i in range(len(self._ts)):
                writer.writerow([
                    f"{self._ts[i]:.3f}",
                    f"{self._vout[i]:.1f}",
                    f"{self._iout[i]:.1f}",
                    f"{self._vin[i]:.1f}",
                    f"{self._temp[i]:.1f}",
                    f"0x{self._status[i]:02X}",
                ])
