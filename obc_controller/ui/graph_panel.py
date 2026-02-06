"""Real-time graph panel using pyqtgraph — dark space theme.

Three separate sub-plots with linked X axes:
  1. Voltage: Vout and Vin (V)
  2. Current: Iout (A)
  3. Temperature: Temp (deg C)

Supports event markers (vertical lines + labels) across all plots.
"""

from __future__ import annotations

import csv
import time
from collections import deque

import numpy as np
import pyqtgraph as pg
from PySide6.QtCore import Qt, QTimer
from PySide6.QtGui import QFont
from PySide6.QtWidgets import (
    QComboBox,
    QFileDialog,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QVBoxLayout,
)

from obc_controller.can_protocol import Message2
from obc_controller.ui.theme import (
    BG_DEEP,
    BORDER,
    CYAN,
    GREEN,
    MAGENTA,
    ORANGE,
    RED,
    TEXT_DIM,
)

# Time-window options (minutes)
WINDOW_OPTIONS = {
    "1 min": 60,
    "5 min": 300,
    "10 min": 600,
    "30 min": 1800,
}

# Severity → line colour
_SEV_COLORS = {
    "info": CYAN,
    "warning": ORANGE,
    "error": RED,
}

# Apply pyqtgraph global dark config
pg.setConfigOptions(
    antialias=True,
    background=BG_DEEP,
    foreground=TEXT_DIM,
)

_MARKER_FONT = QFont("sans-serif", 8)


class GraphPanel(QGroupBox):
    def __init__(self, parent=None):
        super().__init__("Live Graph", parent)

        self._t0 = time.monotonic()
        self._paused = False

        # Ring buffers (store up to 30 min @ 2 Hz = 3600 points max)
        maxlen = 3600
        self._ts: deque[float] = deque(maxlen=maxlen)
        self._vout: deque[float] = deque(maxlen=maxlen)
        self._vin: deque[float] = deque(maxlen=maxlen)
        self._iout: deque[float] = deque(maxlen=maxlen)
        self._temp: deque[float] = deque(maxlen=maxlen)
        self._status: deque[int] = deque(maxlen=maxlen)

        self._window_sec = 600  # default 10 min
        self._data_dirty = False  # flag to avoid redundant np.array() calls

        # Event markers: [(timestamp, label, severity, [lines], text_item)]
        self._markers: list[tuple] = []
        self._marker_data: list[tuple[float, str, str]] = []

        layout = QVBoxLayout(self)

        # --- controls row ---
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

        # --- pyqtgraph widget with 3 rows ---
        self._graphics = pg.GraphicsLayoutWidget()
        layout.addWidget(self._graphics)

        grid_pen = pg.mkPen(BORDER, width=0.5)

        # Plot 1: Voltage (Vout + Vin)
        self._p_volt = self._graphics.addPlot(row=0, col=0, title="Voltage")
        self._p_volt.setLabel("left", "V", color=TEXT_DIM)
        self._p_volt.setLabel("bottom", "s", color=TEXT_DIM)
        self._p_volt.addLegend(brush=(17, 24, 39, 180))
        self._p_volt.showGrid(x=True, y=True, alpha=0.15)
        self._p_volt.getAxis("bottom").setPen(grid_pen)
        self._p_volt.getAxis("left").setPen(grid_pen)
        self._curve_vout = self._p_volt.plot(
            pen=pg.mkPen(CYAN, width=2), name="Vout"
        )
        self._curve_vin = self._p_volt.plot(
            pen=pg.mkPen(MAGENTA, width=2), name="Vin"
        )

        # Plot 2: Current (Iout only)
        self._p_curr = self._graphics.addPlot(row=1, col=0, title="Current")
        self._p_curr.setLabel("left", "A", color=TEXT_DIM)
        self._p_curr.setLabel("bottom", "s", color=TEXT_DIM)
        self._p_curr.addLegend(brush=(17, 24, 39, 180))
        self._p_curr.showGrid(x=True, y=True, alpha=0.15)
        self._p_curr.getAxis("bottom").setPen(grid_pen)
        self._p_curr.getAxis("left").setPen(grid_pen)
        self._curve_iout = self._p_curr.plot(
            pen=pg.mkPen(GREEN, width=2), name="Iout"
        )

        # Plot 3: Temperature
        self._p_temp = self._graphics.addPlot(
            row=2, col=0, title="Temperature"
        )
        self._p_temp.setLabel("left", "\u00b0C", color=TEXT_DIM)
        self._p_temp.setLabel("bottom", "s", color=TEXT_DIM)
        self._p_temp.addLegend(brush=(17, 24, 39, 180))
        self._p_temp.showGrid(x=True, y=True, alpha=0.15)
        self._p_temp.getAxis("bottom").setPen(grid_pen)
        self._p_temp.getAxis("left").setPen(grid_pen)
        self._curve_temp = self._p_temp.plot(
            pen=pg.mkPen(ORANGE, width=2), name="Temp"
        )

        # Link X axes
        self._p_curr.setXLink(self._p_volt)
        self._p_temp.setXLink(self._p_volt)

        # All three plots for marker iteration
        self._plots = (self._p_volt, self._p_curr, self._p_temp)

        # Redraw timer (~15 Hz)
        self._redraw_timer = QTimer(self)
        self._redraw_timer.timeout.connect(self._redraw)
        self._redraw_timer.start(66)

    # ---- public API -------------------------------------------------------

    def add_point(self, msg: Message2) -> None:
        t = time.monotonic() - self._t0
        self._ts.append(t)
        self._vout.append(msg.output_voltage)
        self._vin.append(msg.input_voltage)
        self._iout.append(msg.output_current)
        self._temp.append(msg.temperature)
        self._status.append(msg.status.to_byte())
        self._data_dirty = True

    def add_event_marker(
        self, label: str, severity: str = "info"
    ) -> None:
        """Add a vertical event marker across all 3 plots.

        *severity* is one of ``"info"``, ``"warning"``, ``"error"``.
        The marker line + label is drawn on all three plots (X-linked).
        """
        t = time.monotonic() - self._t0
        color = _SEV_COLORS.get(severity, CYAN)
        pen = pg.mkPen(color, width=1.5, style=Qt.PenStyle.DashLine)

        lines: list = []
        for plot in self._plots:
            line = pg.InfiniteLine(pos=t, angle=90, pen=pen)
            plot.addItem(line)
            lines.append(line)

        # Text label on the voltage plot (top)
        text_item = pg.TextItem(text=label, color=color, anchor=(0, 0))
        text_item.setFont(_MARKER_FONT)
        text_item.setPos(t, 0)
        self._p_volt.addItem(text_item)

        self._markers.append((t, label, severity, lines, text_item))
        self._marker_data.append((t, label, severity))

    # ---- internal ---------------------------------------------------------

    def _redraw(self) -> None:
        if self._paused or len(self._ts) == 0:
            return

        if not self._data_dirty:
            return
        self._data_dirty = False

        ts = np.array(self._ts)
        now = ts[-1]
        cutoff = now - self._window_sec
        mask = ts >= cutoff

        t = ts[mask]
        self._curve_vout.setData(t, np.array(self._vout)[mask])
        self._curve_vin.setData(t, np.array(self._vin)[mask])
        self._curve_iout.setData(t, np.array(self._iout)[mask])
        self._curve_temp.setData(t, np.array(self._temp)[mask])

        # Remove markers that have scrolled out of the visible window
        still_visible: list[tuple] = []
        for marker in self._markers:
            m_t, _, _, lines, text_item = marker
            if m_t < cutoff:
                for plot, line in zip(self._plots, lines):
                    plot.removeItem(line)
                self._p_volt.removeItem(text_item)
            else:
                still_visible.append(marker)
        self._markers = still_visible

        # Reposition marker labels near top of voltage plot
        if self._markers:
            vr = self._p_volt.viewRange()
            y_top = vr[1][0] + 0.92 * (vr[1][1] - vr[1][0])
            for m_t, _, _, _, text_item in self._markers:
                text_item.setPos(m_t, y_top)

    def _on_pause_toggled(self, checked: bool) -> None:
        self._paused = checked
        self._pause_btn.setText("Resume" if checked else "Pause")

    def _on_window_changed(self, text: str) -> None:
        self._window_sec = WINDOW_OPTIONS.get(text, 600)
        self._data_dirty = True

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

        # Remove marker graphics
        for _, _, _, lines, text_item in self._markers:
            for plot, line in zip(self._plots, lines):
                plot.removeItem(line)
            self._p_volt.removeItem(text_item)
        self._markers.clear()
        self._marker_data.clear()

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
                [
                    "timestamp_s",
                    "Vout_V",
                    "Iout_A",
                    "Vin_V",
                    "Temp_C",
                    "status_flags",
                ]
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

            # Append events section
            if self._marker_data:
                writer.writerow([])
                writer.writerow(["# EVENTS"])
                writer.writerow(["timestamp_s", "event_label", "severity"])
                for t, label, sev in self._marker_data:
                    writer.writerow([f"{t:.3f}", label, sev])
