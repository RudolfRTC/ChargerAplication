#include "ui/theme.h"

QString appStyleSheet() {
    return QStringLiteral(R"(
/* global */
* {
    font-family: "Segoe UI", "Roboto", "Helvetica Neue", Arial, sans-serif;
    font-size: 13px;
    color: #e0e7ff;
}
QMainWindow, QWidget#central {
    background: #0a0e1a;
}
QWidget {
    background: transparent;
}

/* group boxes */
QGroupBox {
    background: #1a2332;
    border: 1px solid #2d3748;
    border-radius: 8px;
    margin-top: 14px;
    padding: 14px 10px 10px 10px;
    font-weight: bold;
    color: #00e5ff;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    padding: 2px 12px;
    background: #1a2332;
    border: 1px solid #2d3748;
    border-radius: 4px;
    color: #00e5ff;
    font-size: 12px;
    letter-spacing: 1px;
}

/* labels */
QLabel {
    color: #e0e7ff;
    background: transparent;
}

/* push buttons */
QPushButton {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #1e293b, stop:1 #0f172a);
    border: 1px solid #2d3748;
    border-radius: 6px;
    padding: 7px 18px;
    color: #e0e7ff;
    font-weight: bold;
    min-height: 18px;
}
QPushButton:hover {
    border-color: #00e5ff;
    color: #00e5ff;
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #1a3a4a, stop:1 #0f172a);
}
QPushButton:pressed {
    background: #0f172a;
    border-color: #00e5ff;
}
QPushButton:disabled {
    background: #0d1117;
    color: #475569;
    border-color: #1e293b;
}

/* named buttons */
QPushButton#btn_start {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #166534, stop:1 #14532d);
    border-color: #00e676;
    color: #00e676;
}
QPushButton#btn_start:hover {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #15803d, stop:1 #166534);
    color: #ffffff;
}

QPushButton#btn_stop {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #7f1d1d, stop:1 #450a0a);
    border-color: #ff1744;
    color: #ff1744;
}
QPushButton#btn_stop:hover {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #991b1b, stop:1 #7f1d1d);
    color: #ffffff;
}

QPushButton#btn_heat {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #78350f, stop:1 #451a03);
    border-color: #ff9100;
    color: #ff9100;
}
QPushButton#btn_heat:hover {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #92400e, stop:1 #78350f);
    color: #ffffff;
}

QPushButton#btn_instant {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #4a1d96, stop:1 #2e1065);
    border-color: #7c4dff;
    color: #7c4dff;
}
QPushButton#btn_instant:hover {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #5b21b6, stop:1 #4a1d96);
    color: #ffffff;
}

QPushButton#btn_baud {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #1e3a5f, stop:1 #0c2340);
    border-color: #607D8B;
    color: #90CAF9;
}
QPushButton#btn_baud:hover {
    border-color: #00e5ff;
    color: #00e5ff;
}

QPushButton#btn_connect {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #0e4429, stop:1 #052e16);
    border-color: #00e676;
    color: #00e676;
}
QPushButton#btn_connect:hover {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #166534, stop:1 #0e4429);
    color: #ffffff;
}

QPushButton#btn_disconnect {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #7f1d1d, stop:1 #450a0a);
    border-color: #ff1744;
    color: #ff1744;
}
QPushButton#btn_disconnect:hover {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 #991b1b, stop:1 #7f1d1d);
    color: #ffffff;
}

/* combo box */
QComboBox {
    background: #0f1724;
    border: 1px solid #2d3748;
    border-radius: 4px;
    padding: 4px 8px;
    color: #e0e7ff;
    min-height: 20px;
}
QComboBox:hover {
    border-color: #00e5ff;
}
QComboBox::drop-down {
    border: none;
    width: 24px;
}
QComboBox::down-arrow {
    image: none;
    border-left: 5px solid transparent;
    border-right: 5px solid transparent;
    border-top: 6px solid #00e5ff;
    margin-right: 6px;
}
QComboBox QAbstractItemView {
    background: #0f1724;
    border: 1px solid #2d3748;
    color: #e0e7ff;
    selection-background-color: #1a3a4a;
    selection-color: #00e5ff;
}

/* line edit */
QLineEdit {
    background: #0f1724;
    border: 1px solid #2d3748;
    border-radius: 4px;
    padding: 4px 8px;
    color: #e0e7ff;
    min-height: 20px;
}
QLineEdit:focus {
    border-color: #00e5ff;
}

/* spin box */
QDoubleSpinBox, QSpinBox {
    background: #0f1724;
    border: 1px solid #2d3748;
    border-radius: 4px;
    padding: 4px 8px;
    color: #e0e7ff;
    min-height: 20px;
}
QDoubleSpinBox:focus, QSpinBox:focus {
    border-color: #00e5ff;
}
QDoubleSpinBox::up-button, QSpinBox::up-button,
QDoubleSpinBox::down-button, QSpinBox::down-button {
    background: #1e293b;
    border: none;
    width: 18px;
}
QDoubleSpinBox::up-arrow, QSpinBox::up-arrow {
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-bottom: 5px solid #00e5ff;
}
QDoubleSpinBox::down-arrow, QSpinBox::down-arrow {
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 5px solid #00e5ff;
}

/* check box */
QCheckBox {
    spacing: 8px;
    color: #e0e7ff;
}
QCheckBox::indicator {
    width: 18px;
    height: 18px;
    border: 2px solid #2d3748;
    border-radius: 4px;
    background: #0f1724;
}
QCheckBox::indicator:checked {
    background: #00e5ff;
    border-color: #00e5ff;
}

/* tab widget */
QTabWidget::pane {
    background: #0a0e1a;
    border: 1px solid #2d3748;
    border-radius: 4px;
    top: -1px;
}
QTabBar::tab {
    background: #111827;
    color: #94a3b8;
    border: 1px solid #2d3748;
    border-bottom: none;
    padding: 8px 20px;
    margin-right: 2px;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
    font-weight: bold;
}
QTabBar::tab:selected {
    background: #0a0e1a;
    color: #00e5ff;
    border-color: #00e5ff;
    border-bottom: 2px solid #0a0e1a;
}
QTabBar::tab:hover:!selected {
    color: #e0e7ff;
    background: #1a2332;
}

/* scroll bars */
QScrollArea {
    background: transparent;
    border: none;
}
QScrollBar:vertical {
    background: #0a0e1a;
    width: 10px;
    border-radius: 5px;
}
QScrollBar::handle:vertical {
    background: #334155;
    min-height: 30px;
    border-radius: 5px;
}
QScrollBar::handle:vertical:hover {
    background: #00e5ff;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollBar:horizontal {
    background: #0a0e1a;
    height: 10px;
    border-radius: 5px;
}
QScrollBar::handle:horizontal {
    background: #334155;
    min-width: 30px;
    border-radius: 5px;
}
QScrollBar::handle:horizontal:hover {
    background: #00e5ff;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}

/* splitter */
QSplitter::handle {
    background: #2d3748;
    width: 2px;
}
QSplitter::handle:hover {
    background: #00e5ff;
}

/* log terminal */
QPlainTextEdit {
    background: #050a12;
    color: #00e676;
    border: 1px solid #2d3748;
    border-radius: 6px;
    font-family: "Cascadia Code", "Fira Code", "Consolas", monospace;
    font-size: 12px;
    padding: 6px;
    selection-background-color: #1a3a4a;
}

/* tooltip */
QToolTip {
    background: #1a2332;
    color: #e0e7ff;
    border: 1px solid #00e5ff;
    border-radius: 4px;
    padding: 4px 8px;
}

/* header */
QFrame#header {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 #0a1628, stop:0.5 #111d33, stop:1 #0a1628);
    border-bottom: 2px solid qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 #00e5ff, stop:0.5 #e040fb, stop:1 #00e5ff);
    padding: 8px;
}

/* footer */
QFrame#footer {
    background: #111827;
    border-top: 1px solid #2d3748;
    padding: 4px 12px;
    max-height: 32px;
}

/* sidebar panels */
QWidget#left_sidebar, QWidget#right_sidebar {
    background: #111827;
    border-radius: 8px;
}

/* telemetry labels */
QLabel#tele_value {
    font-size: 18px;
    font-weight: bold;
    color: #00e5ff;
}
QLabel#tele_label {
    color: #94a3b8;
    font-size: 12px;
}
QLabel#alarm_label {
    color: #ff1744;
    font-weight: bold;
    font-size: 13px;
}
QLabel#mode_label {
    font-weight: bold;
    font-size: 13px;
    color: #00e5ff;
}
QLabel#ramp_active {
    color: #e040fb;
    font-weight: bold;
}
QLabel#status_connected {
    color: #00e676;
    font-weight: bold;
}
QLabel#status_disconnected {
    color: #94a3b8;
    font-weight: bold;
}
QLabel#header_title {
    color: #ffffff;
    font-size: 20px;
    font-weight: bold;
    letter-spacing: 2px;
}
QLabel#header_subtitle {
    color: #00e5ff;
    font-size: 11px;
    letter-spacing: 1px;
}
QLabel#footer_text {
    color: #94a3b8;
    font-size: 11px;
}
QLabel#baud_status {
    font-weight: bold;
}
)");
}
