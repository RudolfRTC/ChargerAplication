#!/usr/bin/env python3
"""OBC Charger Controller — desktop application entry point."""

import sys
from pathlib import Path

from PySide6.QtGui import QIcon
from PySide6.QtWidgets import QApplication

from obc_controller.ui.main_window import MainWindow
from obc_controller.ui.theme import QSS

_ASSETS = Path(__file__).parent / "obc_controller" / "ui" / "assets"


def main() -> None:
    app = QApplication(sys.argv)
    app.setApplicationName("OBC Charger Controller")
    app.setStyle("Fusion")
    app.setStyleSheet(QSS)

    icon_path = _ASSETS / "app_icon.png"
    if icon_path.exists():
        app.setWindowIcon(QIcon(str(icon_path)))

    window = MainWindow()
    window.show()

    sys.exit(app.exec())


if __name__ == "__main__":
    main()
