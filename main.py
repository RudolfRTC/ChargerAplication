#!/usr/bin/env python3
"""OBC Charger Controller — desktop application entry point."""

import sys

from PySide6.QtWidgets import QApplication

from obc_controller.ui.main_window import MainWindow


def main() -> None:
    app = QApplication(sys.argv)
    app.setApplicationName("OBC Charger Controller")
    app.setStyle("Fusion")

    window = MainWindow()
    window.show()

    sys.exit(app.exec())


if __name__ == "__main__":
    main()
