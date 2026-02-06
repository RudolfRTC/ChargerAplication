"""Main application window — 3-column space-themed layout."""

from __future__ import annotations

from pathlib import Path

from PySide6.QtCore import Qt, Slot
from PySide6.QtGui import QPixmap
from PySide6.QtWidgets import (
    QDialog,
    QFrame,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QScrollArea,
    QSplitter,
    QTabWidget,
    QVBoxLayout,
    QWidget,
)

from obc_controller.__version__ import __version__
from obc_controller.can_protocol import ChargerControl
from obc_controller.can_worker import BaudrateSwitchWorker, CANWorker
from obc_controller.simulator import Simulator
from obc_controller.ui.connection_panel import ConnectionPanel
from obc_controller.ui.control_panel import ControlPanel
from obc_controller.ui.graph_panel import GraphPanel
from obc_controller.ui.log_panel import LogPanel
from obc_controller.ui.telemetry_panel import TelemetryPanel
from obc_controller.ui.theme import COMPANY, ADDRESS, MADE_BY, CYAN, TEXT_DIM

_ASSETS = Path(__file__).parent / "assets"


class MainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle(f"OBC Charger Controller \u2014 {COMPANY}")
        self.resize(1280, 860)

        self._worker: CANWorker | None = None
        self._simulator: Simulator | None = None
        self._baud_worker: BaudrateSwitchWorker | None = None
        self._sim_mode = False
        self._prev_control = ChargerControl.STOP_OUTPUTTING

        # --- central widget ---
        central = QWidget()
        central.setObjectName("central")
        self.setCentralWidget(central)
        root = QVBoxLayout(central)
        root.setContentsMargins(0, 0, 0, 0)
        root.setSpacing(0)

        # ── header ──────────────────────────────────────────────
        header = QFrame()
        header.setObjectName("header")
        header.setFixedHeight(64)
        h_lay = QHBoxLayout(header)
        h_lay.setContentsMargins(16, 4, 16, 4)

        logo_path = _ASSETS / "rtc_logo.png"
        if logo_path.exists():
            logo_lbl = QLabel()
            pm = QPixmap(str(logo_path)).scaled(
                48, 48, Qt.AspectRatioMode.KeepAspectRatio,
                Qt.TransformationMode.SmoothTransformation,
            )
            logo_lbl.setPixmap(pm)
            h_lay.addWidget(logo_lbl)

        title_col = QVBoxLayout()
        title_col.setSpacing(0)
        title_lbl = QLabel("OBC CHARGER CONTROLLER")
        title_lbl.setObjectName("header_title")
        subtitle_lbl = QLabel(f"{COMPANY}  \u00b7  {ADDRESS}")
        subtitle_lbl.setObjectName("header_subtitle")
        title_col.addWidget(title_lbl)
        title_col.addWidget(subtitle_lbl)
        h_lay.addLayout(title_col, stretch=1)

        about_btn_lbl = QLabel(
            f"<a style='color:{CYAN};' href='#'>About</a>"
        )
        about_btn_lbl.setCursor(Qt.CursorShape.PointingHandCursor)
        about_btn_lbl.linkActivated.connect(self._show_about)
        h_lay.addWidget(about_btn_lbl)

        root.addWidget(header)

        # ── 3-column body ───────────────────────────────────────
        body_splitter = QSplitter(Qt.Orientation.Horizontal)

        # Left sidebar — Connection
        left_widget = QWidget()
        left_widget.setObjectName("left_sidebar")
        left_widget.setMinimumWidth(240)
        left_widget.setMaximumWidth(340)
        left_inner = QVBoxLayout(left_widget)
        left_inner.setContentsMargins(6, 6, 6, 6)
        self._conn_panel = ConnectionPanel()
        left_inner.addWidget(self._conn_panel)
        left_inner.addStretch()

        left_scroll = QScrollArea()
        left_scroll.setWidgetResizable(True)
        left_scroll.setWidget(left_widget)
        left_scroll.setHorizontalScrollBarPolicy(
            Qt.ScrollBarPolicy.ScrollBarAlwaysOff
        )
        body_splitter.addWidget(left_scroll)

        # Center — Tabs (Graphs / Log) + Telemetry
        center = QWidget()
        center_lay = QVBoxLayout(center)
        center_lay.setContentsMargins(4, 4, 4, 4)

        tabs = QTabWidget()
        self._graph_panel = GraphPanel()
        self._log_panel = LogPanel()
        tabs.addTab(self._graph_panel, "Graphs")
        tabs.addTab(self._log_panel, "Log")
        center_lay.addWidget(tabs, stretch=3)

        self._tele_panel = TelemetryPanel()
        center_lay.addWidget(self._tele_panel, stretch=0)

        body_splitter.addWidget(center)

        # Right sidebar — Control
        right_widget = QWidget()
        right_widget.setObjectName("right_sidebar")
        right_widget.setMinimumWidth(260)
        right_widget.setMaximumWidth(380)
        right_inner = QVBoxLayout(right_widget)
        right_inner.setContentsMargins(6, 6, 6, 6)
        self._ctrl_panel = ControlPanel()
        right_inner.addWidget(self._ctrl_panel)
        right_inner.addStretch()

        right_scroll = QScrollArea()
        right_scroll.setWidgetResizable(True)
        right_scroll.setWidget(right_widget)
        right_scroll.setHorizontalScrollBarPolicy(
            Qt.ScrollBarPolicy.ScrollBarAlwaysOff
        )
        body_splitter.addWidget(right_scroll)

        # Splitter proportions: left 20%, center 55%, right 25%
        body_splitter.setStretchFactor(0, 2)
        body_splitter.setStretchFactor(1, 5)
        body_splitter.setStretchFactor(2, 2)
        root.addWidget(body_splitter, stretch=1)

        # ── footer ──────────────────────────────────────────────
        footer = QFrame()
        footer.setObjectName("footer")
        footer.setFixedHeight(28)
        f_lay = QHBoxLayout(footer)
        f_lay.setContentsMargins(12, 0, 12, 0)
        f_left = QLabel(f"{MADE_BY}  \u00b7  {COMPANY}, {ADDRESS}")
        f_left.setObjectName("footer_text")
        f_right = QLabel(f"v{__version__}")
        f_right.setObjectName("footer_text")
        f_lay.addWidget(f_left)
        f_lay.addStretch()
        f_lay.addWidget(f_right)
        root.addWidget(footer)

        # ── wire signals ────────────────────────────────────────
        self._conn_panel.connect_requested.connect(self._on_connect)
        self._conn_panel.disconnect_requested.connect(self._on_disconnect)
        self._conn_panel.baudrate_switch_requested.connect(
            self._on_baudrate_switch
        )
        self._ctrl_panel.control_changed.connect(self._on_control_changed)
        self._ctrl_panel.instant_360v_requested.connect(
            self._on_instant_360v
        )
        self._ctrl_panel.profile_loaded.connect(self._on_profile_loaded)
        self._ctrl_panel.log_message.connect(self._log_panel.append)

    # ---- About dialog ----------------------------------------------------

    def _show_about(self) -> None:
        dlg = QDialog(self)
        dlg.setWindowTitle("About OBC Charger Controller")
        dlg.setFixedSize(420, 320)
        lay = QVBoxLayout(dlg)
        lay.setAlignment(Qt.AlignmentFlag.AlignCenter)
        lay.setSpacing(12)

        logo_path = _ASSETS / "rtc_logo.png"
        if logo_path.exists():
            logo_lbl = QLabel()
            pm = QPixmap(str(logo_path)).scaled(
                96, 96, Qt.AspectRatioMode.KeepAspectRatio,
                Qt.TransformationMode.SmoothTransformation,
            )
            logo_lbl.setPixmap(pm)
            logo_lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
            lay.addWidget(logo_lbl)

        title = QLabel("OBC Charger Controller")
        title.setStyleSheet(
            f"font-size: 18px; font-weight: bold; color: {CYAN};"
        )
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        lay.addWidget(title)

        ver = QLabel(f"Version {__version__}")
        ver.setAlignment(Qt.AlignmentFlag.AlignCenter)
        lay.addWidget(ver)

        company = QLabel(f"{COMPANY}\n{ADDRESS}")
        company.setAlignment(Qt.AlignmentFlag.AlignCenter)
        company.setStyleSheet(f"color: {TEXT_DIM};")
        lay.addWidget(company)

        made = QLabel(MADE_BY)
        made.setAlignment(Qt.AlignmentFlag.AlignCenter)
        made.setStyleSheet(f"color: {CYAN}; font-weight: bold;")
        lay.addWidget(made)

        lay.addStretch()
        dlg.exec()

    # ---- connection management -------------------------------------------

    @Slot(str, str, int, bool)
    def _on_connect(
        self, interface: str, channel: str, bitrate: int, simulate: bool
    ) -> None:
        self._sim_mode = simulate

        if simulate:
            self._log_panel.append("Starting simulation mode \u2026")
            self._simulator = Simulator()
            self._simulator.message2_received.connect(self._on_message2)
            self._simulator.log_message.connect(self._log_panel.append)
            self._simulator.start()
            self._conn_panel.set_connected(True)
            self._ctrl_panel.setEnabled(True)
            # Set initial setpoints in telemetry for sim mode
            self._tele_panel.update_setpoints(
                self._ctrl_panel.get_voltage(),
                self._ctrl_panel.get_current(),
            )
            return

        # Real CAN connection
        self._worker = CANWorker()
        self._worker.set_connection_params(interface, channel, bitrate)

        # Apply initial setpoints + ramp config
        self._worker.set_setpoints(
            self._ctrl_panel.get_voltage(),
            self._ctrl_panel.get_current(),
        )
        self._worker.set_control(self._ctrl_panel.get_control())
        ramp_v, ramp_a = self._ctrl_panel.get_ramp_rates()
        self._worker.set_ramp_config(
            self._ctrl_panel.get_ramp_enabled(), ramp_v, ramp_a
        )
        self._worker.enable_tx(True)

        # Wire worker signals
        self._worker.connected.connect(self._on_worker_connected)
        self._worker.disconnected.connect(self._on_worker_disconnected)
        self._worker.error.connect(self._on_worker_error)
        self._worker.log_message.connect(self._log_panel.append)
        self._worker.message2_received.connect(self._on_message2)
        self._worker.timeout_alarm.connect(self._on_timeout_alarm)
        self._worker.tx_message.connect(self._on_tx_message)
        self._worker.ramp_state.connect(self._on_ramp_state)
        self._worker.health_stats.connect(self._on_health_stats)
        self._worker.status_bit_changed.connect(self._on_status_bit_changed)

        self._worker.start()

    @Slot()
    def _on_disconnect(self) -> None:
        if self._sim_mode and self._simulator is not None:
            self._simulator.request_stop()
            self._simulator.wait(3000)
            self._simulator = None
            self._conn_panel.set_connected(False)
            self._ctrl_panel.setEnabled(False)
            self._tele_panel.clear()
            self._log_panel.append("Simulation stopped.")
            return

        if self._worker is not None:
            self._log_panel.append("Disconnecting \u2026")
            self._worker.request_stop()
            self._worker.wait(10000)
            self._worker = None

    # ---- worker signal handlers ------------------------------------------

    @Slot()
    def _on_worker_connected(self) -> None:
        self._conn_panel.set_connected(True)
        self._ctrl_panel.setEnabled(True)

    @Slot()
    def _on_worker_disconnected(self) -> None:
        self._conn_panel.set_connected(False)
        self._ctrl_panel.setEnabled(False)
        self._tele_panel.clear()
        self._ctrl_panel.update_ramp_display(False, 0, 0)

    @Slot(str)
    def _on_worker_error(self, msg: str) -> None:
        self._log_panel.append(f"ERROR: {msg}")

    @Slot(object)
    def _on_message2(self, msg) -> None:
        self._tele_panel.update_telemetry(msg)
        self._graph_panel.add_point(msg)

    @Slot()
    def _on_timeout_alarm(self) -> None:
        self._tele_panel.set_alarm("ALARM: No Message2 > 5 s!")

    @Slot(object)
    def _on_tx_message(self, msg) -> None:
        self._log_panel.append(
            f"TX Message1: V={msg.voltage_setpoint:.1f}V "
            f"I={msg.current_setpoint:.1f}A "
            f"ctrl={msg.control.name}"
        )
        # Feed SET values to telemetry (ramped or target, whichever is sent)
        self._tele_panel.update_setpoints(
            msg.voltage_setpoint, msg.current_setpoint
        )

    @Slot(bool, float, float)
    def _on_ramp_state(
        self, active: bool, ramped_v: float, ramped_a: float
    ) -> None:
        self._ctrl_panel.update_ramp_display(active, ramped_v, ramped_a)

    @Slot(float, float, float)
    def _on_health_stats(
        self, tx_rate: float, rx_rate: float, last_rx_age: float
    ) -> None:
        self._conn_panel.update_health(tx_rate, rx_rate, last_rx_age)

    @Slot(int, str, bool)
    def _on_status_bit_changed(
        self, bit: int, name: str, is_fault: bool
    ) -> None:
        state = "ON" if is_fault else "OFF"
        severity = "error" if is_fault else "info"
        self._graph_panel.add_event_marker(
            f"FAULT {name} {state}", severity
        )
        self._log_panel.append(f"Status bit {bit} ({name}): {state}")

    # ---- control panel -> worker -----------------------------------------

    @Slot(float, float, int, bool, float, float)
    def _on_control_changed(
        self,
        voltage: float,
        current: float,
        control: int,
        ramp_enabled: bool,
        ramp_v: float,
        ramp_a: float,
    ) -> None:
        ctrl = ChargerControl(control)

        # Detect mode change → add graph marker
        if ctrl != self._prev_control:
            label = {
                ChargerControl.STOP_OUTPUTTING: "STOP",
                ChargerControl.START_CHARGING: "START CHARGING",
                ChargerControl.HEATING_DC_SUPPLY: "HEATING/DC",
            }.get(ctrl, "?")
            self._graph_panel.add_event_marker(f"Mode: {label}", "info")
            self._prev_control = ctrl

        # Update telemetry setpoints in sim mode (no tx_message signal)
        if self._sim_mode:
            self._tele_panel.update_setpoints(voltage, current)

        if self._worker is not None:
            self._worker.set_setpoints(voltage, current)
            self._worker.set_control(ctrl)
            self._worker.set_ramp_config(ramp_enabled, ramp_v, ramp_a)

    @Slot(object)
    def _on_profile_loaded(self, profile) -> None:
        """Reset ramp when a profile is loaded."""
        if self._worker is not None:
            self._worker.reset_ramp()

    # ---- instant 360V / 9A -----------------------------------------------

    @Slot()
    def _on_instant_360v(self) -> None:
        """Apply 360V/9A instant preset to the worker."""
        self._graph_panel.add_event_marker(
            "\u26a1 360V/9A INSTANT", "warning"
        )
        if self._worker is not None:
            self._worker.set_setpoints(360.0, 9.0)
            self._worker.set_control(ChargerControl.HEATING_DC_SUPPLY)
            self._worker.set_ramp_config(False, 5.0, 0.5)
            self._worker.reset_ramp()

    # ---- baudrate switch sequence ----------------------------------------

    @Slot()
    def _on_baudrate_switch(self) -> None:
        if self._worker is None or self._worker._bus is None:
            self._log_panel.append(
                "Cannot switch baudrate: CAN not connected."
            )
            return
        if self._baud_worker is not None and self._baud_worker.isRunning():
            self._log_panel.append(
                "Baudrate switch already in progress."
            )
            return

        self._conn_panel.set_baud_switch_busy(True)
        self._ctrl_panel.setEnabled(False)
        self._graph_panel.add_event_marker("Baud switch START", "info")

        self._baud_worker = BaudrateSwitchWorker(self._worker._bus)
        self._baud_worker.progress.connect(self._on_baud_progress)
        self._baud_worker.finished_ok.connect(self._on_baud_done)
        self._baud_worker.error.connect(self._on_baud_error)
        self._baud_worker.log_message.connect(self._log_panel.append)
        self._baud_worker.start()

    @Slot(int, int)
    def _on_baud_progress(self, step: int, total: int) -> None:
        self._conn_panel.set_baud_switch_progress(step, total)

    @Slot()
    def _on_baud_done(self) -> None:
        self._conn_panel.set_baud_switch_done()
        self._ctrl_panel.setEnabled(True)
        self._baud_worker = None
        self._graph_panel.add_event_marker("Baud switch DONE", "info")

    @Slot(str)
    def _on_baud_error(self, msg: str) -> None:
        self._log_panel.append(f"ERROR: {msg}")
        self._conn_panel.set_baud_switch_busy(False)
        self._ctrl_panel.setEnabled(True)
        self._baud_worker = None

    # ---- window close ----------------------------------------------------

    def closeEvent(self, event) -> None:
        if self._baud_worker is not None and self._baud_worker.isRunning():
            self._baud_worker.request_stop()
            self._baud_worker.wait(5000)
            self._baud_worker = None
        self._on_disconnect()
        super().closeEvent(event)
