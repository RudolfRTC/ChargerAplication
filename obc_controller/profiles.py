"""Profile management: save / load / delete charger profiles to JSON.

Storage location (cross-platform):
  - Windows: %APPDATA%/OBC_Controller/profiles.json
  - Linux:   ~/.config/OBC_Controller/profiles.json
  - macOS:   ~/Library/Application Support/OBC_Controller/profiles.json
"""

from __future__ import annotations

import json
import logging
import os
import platform
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Dict, List, Optional

log = logging.getLogger(__name__)


@dataclass
class Profile:
    name: str
    voltage_set_v: float = 320.0
    current_set_a: float = 50.0
    mode: str = "charging"            # "charging" | "heating"
    ramp_enabled: bool = False
    ramp_rate_v_per_s: float = 5.0
    ramp_rate_a_per_s: float = 0.5


def _config_dir() -> Path:
    """Return the platform-specific config directory."""
    system = platform.system()
    if system == "Windows":
        base = os.environ.get("APPDATA", Path.home() / "AppData" / "Roaming")
        return Path(base) / "OBC_Controller"
    elif system == "Darwin":
        return Path.home() / "Library" / "Application Support" / "OBC_Controller"
    else:
        xdg = os.environ.get("XDG_CONFIG_HOME", Path.home() / ".config")
        return Path(xdg) / "OBC_Controller"


def _profiles_path() -> Path:
    return _config_dir() / "profiles.json"


def load_profiles() -> Dict[str, Profile]:
    """Load all profiles from disk.  Returns empty dict on error."""
    path = _profiles_path()
    if not path.exists():
        return {}
    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
        profiles: Dict[str, Profile] = {}
        for name, data in raw.items():
            data["name"] = name
            profiles[name] = Profile(**data)
        return profiles
    except (json.JSONDecodeError, TypeError, KeyError) as exc:
        log.warning("Corrupt profiles.json, ignoring: %s", exc)
        return {}
    except Exception as exc:
        log.warning("Failed to read profiles: %s", exc)
        return {}


def save_profiles(profiles: Dict[str, Profile]) -> None:
    """Persist all profiles to disk."""
    path = _profiles_path()
    path.parent.mkdir(parents=True, exist_ok=True)
    data = {name: asdict(p) for name, p in profiles.items()}
    # Remove redundant 'name' inside each entry (key is already the name)
    for v in data.values():
        v.pop("name", None)
    path.write_text(json.dumps(data, indent=2), encoding="utf-8")


def save_profile(profile: Profile) -> Dict[str, Profile]:
    """Save or overwrite a single profile, returns the full dict."""
    profiles = load_profiles()
    profiles[profile.name] = profile
    save_profiles(profiles)
    return profiles


def delete_profile(name: str) -> Dict[str, Profile]:
    """Delete a profile by name, returns the remaining dict."""
    profiles = load_profiles()
    profiles.pop(name, None)
    save_profiles(profiles)
    return profiles


def profile_names() -> List[str]:
    """Return a sorted list of profile names."""
    return sorted(load_profiles().keys())
