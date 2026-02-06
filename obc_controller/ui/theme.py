"""Centralized space / sci-fi theme for OBC Charger Controller.

All colours, constants, and QSS live here so panel files stay clean.
"""

from __future__ import annotations

# ── colour palette ──────────────────────────────────────────────────────────
BG_DEEP = "#0a0e1a"
BG_PANEL = "#111827"
BG_CARD = "#1a2332"
BG_INPUT = "#0f1724"
BORDER = "#2d3748"
BORDER_FOCUS = "#00e5ff"

CYAN = "#00e5ff"
MAGENTA = "#e040fb"
VIOLET = "#7c4dff"
GREEN = "#00e676"
RED = "#ff1744"
ORANGE = "#ff9100"

TEXT = "#e0e7ff"
TEXT_DIM = "#94a3b8"
TEXT_HEADING = "#ffffff"

# ── company info ────────────────────────────────────────────────────────────
COMPANY = "RTC d.o.o."
ADDRESS = "Cesta k Dravi 21, 2000 Maribor, Slovenia"
MADE_BY = "Made by RLF"

# ── QSS stylesheet ─────────────────────────────────────────────────────────
QSS = f"""
/* ── global ────────────────────────────────────────────────── */
* {{
    font-family: "Segoe UI", "Roboto", "Helvetica Neue", Arial, sans-serif;
    font-size: 13px;
    color: {TEXT};
}}

QMainWindow, QWidget#central {{
    background: {BG_DEEP};
}}

QWidget {{
    background: transparent;
}}

/* ── group boxes (frosted glass) ──────────────────────────── */
QGroupBox {{
    background: {BG_CARD};
    border: 1px solid {BORDER};
    border-radius: 8px;
    margin-top: 14px;
    padding: 14px 10px 10px 10px;
    font-weight: bold;
    color: {CYAN};
}}

QGroupBox::title {{
    subcontrol-origin: margin;
    subcontrol-position: top left;
    padding: 2px 12px;
    background: {BG_CARD};
    border: 1px solid {BORDER};
    border-radius: 4px;
    color: {CYAN};
    font-size: 12px;
    letter-spacing: 1px;
}}

/* ── labels ───────────────────────────────────────────────── */
QLabel {{
    color: {TEXT};
    background: transparent;
}}

/* ── push buttons ─────────────────────────────────────────── */
QPushButton {{
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #1e293b, stop:1 #0f172a);
    border: 1px solid {BORDER};
    border-radius: 6px;
    padding: 7px 18px;
    color: {TEXT};
    font-weight: bold;
    min-height: 18px;
}}

QPushButton:hover {{
    border-color: {CYAN};
    color: {CYAN};
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #1a3a4a, stop:1 #0f172a);
}}

QPushButton:pressed {{
    background: #0f172a;
    border-color: {CYAN};
}}

QPushButton:disabled {{
    background: #0d1117;
    color: #475569;
    border-color: #1e293b;
}}

/* ── named buttons ────────────────────────────────────────── */
QPushButton#btn_start {{
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #166534, stop:1 #14532d);
    border-color: {GREEN};
    color: {GREEN};
}}
QPushButton#btn_start:hover {{
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #15803d, stop:1 #166534);
    color: #ffffff;
}}

QPushButton#btn_stop {{
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #7f1d1d, stop:1 #450a0a);
    border-color: {RED};
    color: {RED};
}}
QPushButton#btn_stop:hover {{
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #991b1b, stop:1 #7f1d1d);
    color: #ffffff;
}}

QPushButton#btn_heat {{
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #78350f, stop:1 #451a03);
    border-color: {ORANGE};
    color: {ORANGE};
}}
QPushButton#btn_heat:hover {{
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #92400e, stop:1 #78350f);
    color: #ffffff;
}}

QPushButton#btn_instant {{
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #4a1d96, stop:1 #2e1065);
    border-color: {VIOLET};
    color: {VIOLET};
}}
QPushButton#btn_instant:hover {{
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #5b21b6, stop:1 #4a1d96);
    color: #ffffff;
}}

QPushButton#btn_baud {{
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #1e3a5f, stop:1 #0c2340);
    border-color: #607D8B;
    color: #90CAF9;
}}
QPushButton#btn_baud:hover {{
    border-color: {CYAN};
    color: {CYAN};
}}

QPushButton#btn_connect {{
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #0e4429, stop:1 #052e16);
    border-color: {GREEN};
    color: {GREEN};
}}
QPushButton#btn_connect:hover {{
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #166534, stop:1 #0e4429);
    color: #ffffff;
}}

QPushButton#btn_disconnect {{
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #7f1d1d, stop:1 #450a0a);
    border-color: {RED};
    color: {RED};
}}
QPushButton#btn_disconnect:hover {{
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #991b1b, stop:1 #7f1d1d);
    color: #ffffff;
}}

/* ── combo box ────────────────────────────────────────────── */
QComboBox {{
    background: {BG_INPUT};
    border: 1px solid {BORDER};
    border-radius: 4px;
    padding: 4px 8px;
    color: {TEXT};
    min-height: 20px;
}}

QComboBox:hover {{
    border-color: {CYAN};
}}

QComboBox::drop-down {{
    border: none;
    width: 24px;
}}

QComboBox::down-arrow {{
    image: none;
    border-left: 5px solid transparent;
    border-right: 5px solid transparent;
    border-top: 6px solid {CYAN};
    margin-right: 6px;
}}

QComboBox QAbstractItemView {{
    background: {BG_INPUT};
    border: 1px solid {BORDER};
    color: {TEXT};
    selection-background-color: #1a3a4a;
    selection-color: {CYAN};
}}

/* ── line edit ────────────────────────────────────────────── */
QLineEdit {{
    background: {BG_INPUT};
    border: 1px solid {BORDER};
    border-radius: 4px;
    padding: 4px 8px;
    color: {TEXT};
    min-height: 20px;
}}

QLineEdit:focus {{
    border-color: {CYAN};
}}

/* ── spin box ─────────────────────────────────────────────── */
QDoubleSpinBox, QSpinBox {{
    background: {BG_INPUT};
    border: 1px solid {BORDER};
    border-radius: 4px;
    padding: 4px 8px;
    color: {TEXT};
    min-height: 20px;
}}

QDoubleSpinBox:focus, QSpinBox:focus {{
    border-color: {CYAN};
}}

QDoubleSpinBox::up-button, QSpinBox::up-button,
QDoubleSpinBox::down-button, QSpinBox::down-button {{
    background: #1e293b;
    border: none;
    width: 18px;
}}

QDoubleSpinBox::up-arrow, QSpinBox::up-arrow {{
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-bottom: 5px solid {CYAN};
}}

QDoubleSpinBox::down-arrow, QSpinBox::down-arrow {{
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 5px solid {CYAN};
}}

/* ── check box ────────────────────────────────────────────── */
QCheckBox {{
    spacing: 8px;
    color: {TEXT};
}}

QCheckBox::indicator {{
    width: 18px;
    height: 18px;
    border: 2px solid {BORDER};
    border-radius: 4px;
    background: {BG_INPUT};
}}

QCheckBox::indicator:checked {{
    background: {CYAN};
    border-color: {CYAN};
}}

/* ── tab widget ───────────────────────────────────────────── */
QTabWidget::pane {{
    background: {BG_DEEP};
    border: 1px solid {BORDER};
    border-radius: 4px;
    top: -1px;
}}

QTabBar::tab {{
    background: {BG_PANEL};
    color: {TEXT_DIM};
    border: 1px solid {BORDER};
    border-bottom: none;
    padding: 8px 20px;
    margin-right: 2px;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
    font-weight: bold;
}}

QTabBar::tab:selected {{
    background: {BG_DEEP};
    color: {CYAN};
    border-color: {CYAN};
    border-bottom: 2px solid {BG_DEEP};
}}

QTabBar::tab:hover:!selected {{
    color: {TEXT};
    background: #1a2332;
}}

/* ── scroll area / scroll bars ───────────────────────────── */
QScrollArea {{
    background: transparent;
    border: none;
}}

QScrollBar:vertical {{
    background: {BG_DEEP};
    width: 10px;
    margin: 0;
    border-radius: 5px;
}}

QScrollBar::handle:vertical {{
    background: #334155;
    min-height: 30px;
    border-radius: 5px;
}}

QScrollBar::handle:vertical:hover {{
    background: {CYAN};
}}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {{
    height: 0;
}}

QScrollBar:horizontal {{
    background: {BG_DEEP};
    height: 10px;
    border-radius: 5px;
}}

QScrollBar::handle:horizontal {{
    background: #334155;
    min-width: 30px;
    border-radius: 5px;
}}

QScrollBar::handle:horizontal:hover {{
    background: {CYAN};
}}

QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {{
    width: 0;
}}

/* ── splitter ─────────────────────────────────────────────── */
QSplitter::handle {{
    background: {BORDER};
    width: 2px;
}}

QSplitter::handle:hover {{
    background: {CYAN};
}}

/* ── plain text edit (log terminal) ──────────────────────── */
QPlainTextEdit {{
    background: #050a12;
    color: {GREEN};
    border: 1px solid {BORDER};
    border-radius: 6px;
    font-family: "Cascadia Code", "Fira Code", "Consolas", monospace;
    font-size: 12px;
    padding: 6px;
    selection-background-color: #1a3a4a;
}}

/* ── tooltip ──────────────────────────────────────────────── */
QToolTip {{
    background: {BG_CARD};
    color: {TEXT};
    border: 1px solid {CYAN};
    border-radius: 4px;
    padding: 4px 8px;
}}

/* ── header frame ────────────────────────────────────────── */
QFrame#header {{
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 #0a1628, stop:0.5 #111d33, stop:1 #0a1628);
    border-bottom: 2px solid qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 {CYAN}, stop:0.5 {MAGENTA}, stop:1 {CYAN});
    padding: 8px;
}}

/* ── footer frame ────────────────────────────────────────── */
QFrame#footer {{
    background: {BG_PANEL};
    border-top: 1px solid {BORDER};
    padding: 4px 12px;
    max-height: 32px;
}}

/* ── sidebar panels ──────────────────────────────────────── */
QWidget#left_sidebar, QWidget#right_sidebar {{
    background: {BG_PANEL};
    border-radius: 8px;
}}

/* ── telemetry value labels ──────────────────────────────── */
QLabel#tele_value {{
    font-size: 18px;
    font-weight: bold;
    color: {CYAN};
}}

QLabel#tele_label {{
    color: {TEXT_DIM};
    font-size: 12px;
}}

QLabel#alarm_label {{
    color: {RED};
    font-weight: bold;
    font-size: 13px;
}}

QLabel#mode_label {{
    font-weight: bold;
    font-size: 13px;
    color: {CYAN};
}}

QLabel#ramp_active {{
    color: {MAGENTA};
    font-weight: bold;
}}

QLabel#status_connected {{
    color: {GREEN};
    font-weight: bold;
}}

QLabel#status_disconnected {{
    color: {TEXT_DIM};
    font-weight: bold;
}}

QLabel#header_title {{
    color: {TEXT_HEADING};
    font-size: 20px;
    font-weight: bold;
    letter-spacing: 2px;
}}

QLabel#header_subtitle {{
    color: {CYAN};
    font-size: 11px;
    letter-spacing: 1px;
}}

QLabel#footer_text {{
    color: {TEXT_DIM};
    font-size: 11px;
}}

QLabel#baud_status {{
    font-weight: bold;
}}
"""
