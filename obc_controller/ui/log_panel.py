"""Log panel: scrollable log view with save functionality."""

from __future__ import annotations

import time
from pathlib import Path

from PySide6.QtWidgets import (
    QGroupBox,
    QHBoxLayout,
    QPlainTextEdit,
    QPushButton,
    QVBoxLayout,
    QFileDialog,
)


class LogPanel(QGroupBox):
    def __init__(self, parent=None):
        super().__init__("Log", parent)
        layout = QVBoxLayout(self)

        self._text_edit = QPlainTextEdit()
        self._text_edit.setReadOnly(True)
        self._text_edit.setMaximumBlockCount(5000)
        layout.addWidget(self._text_edit)

        btn_row = QHBoxLayout()
        self._save_btn = QPushButton("Save Log")
        self._clear_btn = QPushButton("Clear")
        btn_row.addWidget(self._save_btn)
        btn_row.addWidget(self._clear_btn)
        btn_row.addStretch()
        layout.addLayout(btn_row)

        self._save_btn.clicked.connect(self._save_log)
        self._clear_btn.clicked.connect(self._text_edit.clear)

    def append(self, text: str) -> None:
        ts = time.strftime("%H:%M:%S")
        self._text_edit.appendPlainText(f"[{ts}] {text}")

    def _save_log(self) -> None:
        path, _ = QFileDialog.getSaveFileName(
            self,
            "Save Log",
            f"obc_log_{time.strftime('%Y%m%d_%H%M%S')}.txt",
            "Text files (*.txt);;All files (*)",
        )
        if path:
            Path(path).write_text(
                self._text_edit.toPlainText(), encoding="utf-8"
            )
