"""Main application window — wires all panels together."""

from __future__ import annotations

from PySide6.QtCore import Slot
from PySide6.QtWidgets import (
    QHBoxLayout,
    QMainWindow,
    QSplitter,
    QTabWidget,
    QVBoxLayout,
    QWidget,
)

from obc_controller.can_protocol import ChargerControl
from obc_controller.can_worker import CANWorker
from obc_controller.simulator import Simulator
from obc_controller.ui.connection_panel import ConnectionPanel
from obc_controller.ui.control_panel import ControlPanel
from obc_controller.ui.graph_panel import GraphPanel
from obc_controller.ui.log_panel import LogPanel
from obc_controller.ui.telemetry_panel import TelemetryPanel


class MainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("OBC Charger Controller")
        self.resize(1100, 750)

        self._worker: CANWorker | None = None
        self._simulator: Simulator | None = None
        self._sim_mode = False

        # --- build UI ---
        central = QWidget()
        self.setCentralWidget(central)
        root_layout = QVBoxLayout(central)

        # Connection panel (always visible at top)
        self._conn_panel = ConnectionPanel()
        root_layout.addWidget(self._conn_panel)

        # Tab widget with two tabs: Dashboard and Graph
        tabs = QTabWidget()
        root_layout.addWidget(tabs, stretch=1)

        # --- Dashboard tab ---
        dash_widget = QWidget()
        dash_layout = QHBoxLayout(dash_widget)

        left_col = QVBoxLayout()
        self._ctrl_panel = ControlPanel()
        self._tele_panel = TelemetryPanel()
        left_col.addWidget(self._ctrl_panel)
        left_col.addWidget(self._tele_panel)
        left_col.addStretch()

        self._log_panel = LogPanel()

        splitter = QSplitter()
        left_widget = QWidget()
        left_widget.setLayout(left_col)
        splitter.addWidget(left_widget)
        splitter.addWidget(self._log_panel)
        splitter.setStretchFactor(0, 1)
        splitter.setStretchFactor(1, 1)
        dash_layout.addWidget(splitter)

        tabs.addTab(dash_widget, "Dashboard")

        # --- Graph tab ---
        self._graph_panel = GraphPanel()
        tabs.addTab(self._graph_panel, "Graph")

        # --- wire signals ---
        self._conn_panel.connect_requested.connect(self._on_connect)
        self._conn_panel.disconnect_requested.connect(self._on_disconnect)
        self._ctrl_panel.control_changed.connect(self._on_control_changed)

    # ---- connection management -------------------------------------------

    @Slot(str, str, int, bool)
    def _on_connect(
        self, interface: str, channel: str, bitrate: int, simulate: bool
    ) -> None:
        self._sim_mode = simulate

        if simulate:
            self._log_panel.append("Starting simulation mode …")
            self._simulator = Simulator()
            self._simulator.message2_received.connect(self._on_message2)
            self._simulator.log_message.connect(self._log_panel.append)
            self._simulator.start()
            self._conn_panel.set_connected(True)
            self._ctrl_panel.setEnabled(True)
            return

        # Real CAN connection
        self._worker = CANWorker()
        self._worker.set_connection_params(interface, channel, bitrate)

        # Apply initial setpoints
        self._worker.set_setpoints(
            self._ctrl_panel.get_voltage(),
            self._ctrl_panel.get_current(),
        )
        self._worker.set_control(self._ctrl_panel.get_control())
        self._worker.enable_tx(True)

        # Wire worker signals
        self._worker.connected.connect(self._on_worker_connected)
        self._worker.disconnected.connect(self._on_worker_disconnected)
        self._worker.error.connect(self._on_worker_error)
        self._worker.log_message.connect(self._log_panel.append)
        self._worker.message2_received.connect(self._on_message2)
        self._worker.timeout_alarm.connect(self._on_timeout_alarm)
        self._worker.tx_message.connect(self._on_tx_message)

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
            self._log_panel.append("Disconnecting …")
            self._worker.request_stop()
            self._worker.wait(10000)  # wait for safe-stop
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

    # ---- control panel -> worker -----------------------------------------

    @Slot(float, float, int)
    def _on_control_changed(
        self, voltage: float, current: float, control: int
    ) -> None:
        if self._worker is not None:
            self._worker.set_setpoints(voltage, current)
            self._worker.set_control(ChargerControl(control))

    # ---- window close ----------------------------------------------------

    def closeEvent(self, event) -> None:
        self._on_disconnect()
        super().closeEvent(event)
