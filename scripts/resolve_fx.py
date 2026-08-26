#!/usr/bin/env python3
"""
Resolve WLED effect IDs by official English name from the live device.

Hard-coded FX IDs break across WLED versions (e.g. skill docs said fx=10 Forest,
but WLED 16.0.1 fx=10 is Scan). Always prefer name → index from GET /json/effects.
"""
from __future__ import annotations

import json
import os
import urllib.request
from typing import Iterable

# Smooth effects preferred for light recipes (names must match /json/effects).
SMOOTH_BY_THEME: dict[str, list[str]] = {
    "warm": ["Sunrise", "Fire 2012", "Breathe", "Fade", "Dissolve"],
    "nature": ["Fade", "Dissolve", "Breathe", "Aurora", "Pacifica", "Lake"],
    "cool": ["Aurora", "Pacifica", "Fade", "Dissolve", "Breathe"],
    "party": ["Fade", "Dissolve", "Breathe", "Rainbow", "Bpm"],
    "default": ["Fade", "Dissolve", "Breathe", "Rainbow"],
}

# Never pick these for skill recipes (flashy / reserved).
FORBIDDEN_NAMES = {
    "Blink", "Strobe", "Strobe Rainbow", "Strobe Mega", "Blink Rainbow",
    "RSVD", "Lightning", "Chase Flash", "Chase Flash Rnd",
}


def fetch_effects(host: str, timeout: float = 2.5) -> list[str]:
    clean = host.strip()
    for prefix in ("http://", "https://"):
        if clean.startswith(prefix):
            clean = clean[len(prefix):]
    clean = clean.split("/")[0].split(":")[0]
    if not clean:
        return []
    url = f"http://{clean}/json/effects"
    req = urllib.request.Request(url, method="GET")
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        data = json.loads(resp.read().decode())
    if not isinstance(data, list):
        return []
    return [str(x) for x in data]


def index_by_name(effects: list[str]) -> dict[str, int]:
    out: dict[str, int] = {}
    for i, name in enumerate(effects):
        key = name.strip()
        if key and key not in out:
            out[key] = i
    return out


def resolve_fx_id(
    effects: list[str],
    preferred_names: Iterable[str],
    fallback: int = 12,
) -> tuple[int, str]:
    """Return (fx_id, matched_name). Skips RSVD / forbidden."""
    by_name = index_by_name(effects)
    for name in preferred_names:
        n = name.strip()
        if n in FORBIDDEN_NAMES:
            continue
        if n in by_name:
            fx = by_name[n]
            if effects[fx].startswith("RSVD"):
                continue
            return fx, effects[fx]
    # fallback: Fade if present, else clamp
    if "Fade" in by_name:
        return by_name["Fade"], "Fade"
    fx = max(0, min(fallback, len(effects) - 1)) if effects else fallback
    return fx, effects[fx] if effects and fx < len(effects) else "Fade"


def resolve_theme_fx(effects: list[str], theme: str) -> tuple[int, str]:
    names = SMOOTH_BY_THEME.get(theme) or SMOOTH_BY_THEME["default"]
    return resolve_fx_id(effects, names)


def load_effects_for_host(host: str | None, cache_path: str | None = None) -> list[str]:
    if host:
        try:
            effects = fetch_effects(host)
            if effects:
                if cache_path:
                    os.makedirs(os.path.dirname(cache_path) or ".", exist_ok=True)
                    with open(cache_path, "w", encoding="utf-8") as f:
                        json.dump({"host": host, "effects": effects}, f)
                return effects
        except Exception:
            pass
    if cache_path and os.path.exists(cache_path):
        try:
            with open(cache_path, "r", encoding="utf-8") as f:
                cached = json.load(f)
            effects = cached.get("effects") or []
            if effects:
                return effects
        except Exception:
            pass
    return []


if __name__ == "__main__":
    import sys
    host = sys.argv[1] if len(sys.argv) > 1 else ""
    theme = sys.argv[2] if len(sys.argv) > 2 else "nature"
    effects = load_effects_for_host(host) if host else []
    if not effects:
        print(json.dumps({"success": False, "message": "usage: resolve_fx.py <host> [theme]"}))
        raise SystemExit(1)
    fx, name = resolve_theme_fx(effects, theme)
    print(json.dumps({"success": True, "theme": theme, "fx": fx, "name": name, "count": len(effects)}))
