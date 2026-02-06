"""
CAN bus worker running in a QThread.

Handles:
  - Connecting / disconnecting the python-can bus
  - Periodic Message1 TX (500 ms) with optional ramp (soft-start)
  - Message2 RX with timeout alarm (5 s)
  - Safe-stop on disconnect (sends Control=1 for several cycles)
"""

from __future__ import annotations

import logging
import time
from typing import Optional

import can

from PySide6.QtCore import QThread, Signal, QMutex, QMutexLocker

from obc_controller.can_protocol import (
    MSG1_ID,
    MSG2_ID,
    CYCLE_MS,
    TIMEOUT_S,
    ChargerControl,
    Message1,
    Message2,
)

log = logging.getLogger(__name__)

SAFE_STOP_CYCLES = 5  # Send Control=1 this many times before disconnect


def _move_towards(current: float, target: float, max_step: float) -> float:
    """Move *current* towards *target* by at most *max_step*."""
    diff = target - current
    if abs(diff) <= max_step:
        return target
    return current + (max_step if diff > 0 else -max_step)


class CANWorker(QThread):
    """Background thread that owns the python-can bus object."""

    # Signals emitted to the UI thread
    connected = Signal()
    disconnected = Signal()
    error = Signal(str)
    log_message = Signal(str)
    message2_received = Signal(object)  # Message2 dataclass
    timeout_alarm = Signal()
    tx_message = Signal(object)         # Message1 sent (for log)
    ramp_state = Signal(bool, float, float)  # active, ramped_v, ramped_a

    def __init__(self, parent=None):
        super().__init__(parent)
        self._bus: Optional[can.Bus] = None
        self._running = False
        self._mutex = QMutex()

        # TX target state (set from UI thread, protected by mutex)
        self._target_voltage: float = 0.0
        self._target_current: float = 0.0
        self._control: ChargerControl = ChargerControl.STOP_OUTPUTTING
        self._tx_enabled = False

        # Ramp config (set from UI thread, protected by mutex)
        self._ramp_enabled: bool = False
        self._ramp_rate_v: float = 5.0   # V/s
        self._ramp_rate_a: float = 0.5   # A/s
        self._ramp_reset_flag: bool = False

        # Connection parameters
        self._interface = "pcan"
        self._channel = "PCAN_USBBUS1"
        self._bitrate = 250000

    # ---- public setters (called from UI thread) --------------------------

    def set_connection_params(
        self, interface: str, channel: str, bitrate: int
    ) -> None:
        self._interface = interface
        self._channel = channel
        self._bitrate = bitrate

    def set_setpoints(self, voltage: float, current: float) -> None:
        with QMutexLocker(self._mutex):
            self._target_voltage = voltage
            self._target_current = current

    def set_control(self, ctrl: ChargerControl) -> None:
        with QMutexLocker(self._mutex):
            self._control = ctrl

    def set_ramp_config(
        self, enabled: bool, rate_v: float, rate_a: float
    ) -> None:
        with QMutexLocker(self._mutex):
            self._ramp_enabled = enabled
            self._ramp_rate_v = rate_v
            self._ramp_rate_a = rate_a

    def reset_ramp(self) -> None:
        """Request ramp state machine to restart from 0."""
        with QMutexLocker(self._mutex):
            self._ramp_reset_flag = True

    def enable_tx(self, enabled: bool) -> None:
        with QMutexLocker(self._mutex):
            self._tx_enabled = enabled

    def request_stop(self) -> None:
        self._running = False

    # ---- thread entry point ----------------------------------------------

    def run(self) -> None:  # noqa: C901
        # --- connect ---
        try:
            kwargs: dict = {
                "interface": self._interface,
                "channel": self._channel,
                "bitrate": self._bitrate,
            }
            if self._interface == "pcan":
                self.log_message.emit(
                    f"Connecting PCAN channel={self._channel} "
                    f"bitrate={self._bitrate} …"
                )
            self._bus = can.Bus(**kwargs)
            self.log_message.emit("CAN bus connected.")
            self.connected.emit()
        except Exception as exc:
            msg = f"CAN connect failed: {exc}"
            if self._interface == "pcan" and "bitrate" in str(exc).lower():
                msg += (
                    "\nNote: Bitrate is configured in PCAN driver / PCAN-View."
                )
            self.log_message.emit(msg)
            self.error.emit(msg)
            self.disconnected.emit()
            return

        self._running = True
        last_tx_time = 0.0
        last_rx_time = time.monotonic()
        alarm_active = False

        # Ramp internal state
        ramped_v = 0.0
        ramped_a = 0.0
        prev_control = ChargerControl.STOP_OUTPUTTING

        try:
            while self._running:
                now = time.monotonic()

                # ---- read UI state under lock ----------------------------
                with QMutexLocker(self._mutex):
                    tx_en = self._tx_enabled
                    ctrl = self._control
                    tgt_v = self._target_voltage
                    tgt_a = self._target_current
                    ramp_en = self._ramp_enabled
                    ramp_rv = self._ramp_rate_v
                    ramp_ra = self._ramp_rate_a
                    do_reset = self._ramp_reset_flag
                    self._ramp_reset_flag = False

                # ---- ramp reset triggers ---------------------------------
                if do_reset:
                    ramped_v = 0.0
                    ramped_a = 0.0

                # Transition from STOP to active mode -> reset ramp
                if (
                    ctrl != ChargerControl.STOP_OUTPUTTING
                    and prev_control == ChargerControl.STOP_OUTPUTTING
                ):
                    ramped_v = 0.0
                    ramped_a = 0.0
                prev_control = ctrl

                # ---- TX --------------------------------------------------
                if tx_en and (now - last_tx_time) >= (CYCLE_MS / 1000.0):
                    dt = now - last_tx_time if last_tx_time > 0 else CYCLE_MS / 1000.0

                    # Compute actual setpoints
                    if (
                        ctrl == ChargerControl.STOP_OUTPUTTING
                        or not ramp_en
                    ):
                        # No ramp: send target directly
                        send_v = tgt_v
                        send_a = tgt_a
                        ramp_active = False
                    else:
                        # Ramp towards target
                        step_v = ramp_rv * dt
                        step_a = ramp_ra * dt
                        ramped_v = _move_towards(ramped_v, tgt_v, step_v)
                        ramped_a = _move_towards(ramped_a, tgt_a, step_a)
                        # Round to 0.1 (CAN resolution)
                        send_v = round(ramped_v, 1)
                        send_a = round(ramped_a, 1)
                        ramp_active = (
                            send_v != round(tgt_v, 1)
                            or send_a != round(tgt_a, 1)
                        )

                    msg1 = Message1(
                        voltage_setpoint=send_v,
                        current_setpoint=send_a,
                        control=ctrl,
                    )
                    frame = can.Message(
                        arbitration_id=MSG1_ID,
                        data=msg1.encode(),
                        is_extended_id=True,
                    )
                    try:
                        self._bus.send(frame)
                        last_tx_time = now
                        self.tx_message.emit(msg1)
                        self.ramp_state.emit(ramp_active, send_v, send_a)
                    except can.CanError as exc:
                        self.log_message.emit(f"TX error: {exc}")

                # ---- RX --------------------------------------------------
                try:
                    frame = self._bus.recv(timeout=0.05)
                except can.CanError as exc:
                    self.log_message.emit(f"RX error: {exc}")
                    frame = None

                if frame is not None and frame.arbitration_id == MSG2_ID:
                    try:
                        msg2 = Message2.decode(frame.data)
                        self.message2_received.emit(msg2)
                        last_rx_time = now
                        if alarm_active:
                            alarm_active = False
                            self.log_message.emit(
                                "Message2 received — timeout cleared."
                            )
                    except Exception as exc:
                        self.log_message.emit(
                            f"Message2 decode error: {exc}"
                        )

                # ---- timeout check ---------------------------------------
                if tx_en and (now - last_rx_time) > TIMEOUT_S:
                    if not alarm_active:
                        alarm_active = True
                        self.timeout_alarm.emit()
                        self.log_message.emit(
                            "ALARM: No Message2 for > 5 s!"
                        )

        finally:
            self._safe_stop()
            self._close_bus()
            self.disconnected.emit()

    # ---- internal helpers ------------------------------------------------

    def _safe_stop(self) -> None:
        """Send Control=1 (stop) for several cycles before shutting down."""
        if self._bus is None:
            return
        self.log_message.emit("Safe-stop: sending Control=STOP …")
        msg1 = Message1(
            voltage_setpoint=0,
            current_setpoint=0,
            control=ChargerControl.STOP_OUTPUTTING,
        )
        frame = can.Message(
            arbitration_id=MSG1_ID,
            data=msg1.encode(),
            is_extended_id=True,
        )
        for _ in range(SAFE_STOP_CYCLES):
            try:
                self._bus.send(frame)
            except can.CanError:
                break
            time.sleep(CYCLE_MS / 1000.0)
        self.log_message.emit("Safe-stop complete.")

    def _close_bus(self) -> None:
        if self._bus is not None:
            try:
                self._bus.shutdown()
            except Exception:
                pass
            self._bus = None
            self.log_message.emit("CAN bus closed.")
