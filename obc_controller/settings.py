"""User settings: persistent JSON key-value store.

Storage: same config directory as profiles.json.
"""

from __future__ import annotations

import json
import logging
from pathlib import Path

from obc_controller.profiles import _config_dir

log = logging.getLogger(__name__)


def _settings_path() -> Path:
    return _config_dir() / "settings.json"


def load_settings() -> dict:
    """Load all settings from disk.  Returns empty dict on error."""
    path = _settings_path()
    if not path.exists():
        return {}
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        log.warning("Failed to read settings: %s", exc)
        return {}


def save_setting(key: str, value: object) -> None:
    """Save a single setting key."""
    settings = load_settings()
    settings[key] = value
    path = _settings_path()
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(settings, indent=2), encoding="utf-8")
