#!/usr/bin/env python3
"""
Export a doubao-wled-recipe JSON into SYMI Ambient light-recipe import format.

SYMI Ambient home card rules:
  - WLED native Effect: line1 = Chinese, line2 = official English from /json/effects
  - Skill-created custom recipe: line1 = "光配方", line2 = customer recipe name

Usage:
    python3 export_symi_ambient.py recipes/jasmine.json
    python3 export_symi_ambient.py recipes/jasmine.json --name "茉莉花盛开"
    python3 export_symi_ambient.py recipes/jasmine.json --out /tmp/symi_recipe.json

Then in SYMI Ambient: 场景管理 → 导入 → select the generated JSON
(or merge into a full catalog export that contains homeIds + recipes).
"""
from __future__ import annotations

import argparse
import json
import os
import sys
import time
from datetime import datetime

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SKILL_DIR = os.path.dirname(SCRIPT_DIR)


def build_state_json(recipe: dict) -> str:
    fx = int(recipe.get("fx", 9))
    sx = int(recipe.get("sx", 70))
    ix = int(recipe.get("ix", 100))
    bri = int(recipe.get("bri", 200))
    pal = int(recipe.get("pal", 0))
    colors = recipe.get("colors") or []
    cols = []
    for i in range(3):
        if i < len(colors) and isinstance(colors[i], (list, tuple)) and len(colors[i]) >= 3:
            cols.append([int(colors[i][0]), int(colors[i][1]), int(colors[i][2])])
        else:
            cols.append([0, 0, 0])
    state = {
        "on": True,
        "bri": bri,
        "live": False,
        "lor": 0,
        "transition": 0,
        "mainseg": 0,
        "seg": [{"id": 0, "fx": fx, "pal": pal, "sx": sx, "ix": ix, "col": cols}],
    }
    return json.dumps(state, ensure_ascii=False, separators=(",", ":"))


def to_symi_recipe(recipe: dict, display_name: str | None = None) -> dict:
    name = (display_name or recipe.get("name") or "未命名光配方").strip()
    rid = recipe.get("id") or f"custom_{int(time.time())}"
    return {
        "id": rid,
        "name": name,
        "kind": "custom",
        "linePrimary": "光配方",
        "lineSecondary": name,
        "stateJson": build_state_json(recipe),
        # Keep skill provenance for debugging / re-import
        "format": "doubao-wled-recipe-v1",
        "source_fx": recipe.get("fx"),
        "source_theme": recipe.get("theme", ""),
        "exported_at": datetime.now().isoformat(timespec="seconds"),
    }


def to_catalog(symi_recipe: dict, show_on_home: bool = True) -> dict:
    """Full SYMI Ambient import catalog (LightRecipeCatalog.importJson)."""
    return {
        "version": 1,
        "homeIds": [symi_recipe["id"]] if show_on_home else [],
        "recipes": [symi_recipe],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Export skill recipe → SYMI Ambient import JSON")
    parser.add_argument("recipe", help="Path to doubao-wled-recipe JSON")
    parser.add_argument("--name", help="Customer-facing recipe name (line 2 on home card)")
    parser.add_argument("--out", help="Output path (default: recipes/symi_<name>.json)")
    parser.add_argument("--no-home", action="store_true", help="Do not put recipe on homeIds")
    parser.add_argument("--single", action="store_true", help="Emit single recipe object only")
    args = parser.parse_args()

    path = args.recipe
    if not os.path.isabs(path):
        cand = os.path.join(SKILL_DIR, path)
        path = cand if os.path.exists(cand) else args.recipe
    if not os.path.exists(path):
        print(json.dumps({"success": False, "message": f"not found: {args.recipe}"}, ensure_ascii=False))
        return 1

    with open(path, "r", encoding="utf-8") as f:
        recipe = json.load(f)

    symi = to_symi_recipe(recipe, args.name)
    payload = symi if args.single else to_catalog(symi, show_on_home=not args.no_home)

    out = args.out
    if not out:
        safe = "".join(c if c.isalnum() or c in "-_" else "_" for c in symi["name"])[:40]
        out = os.path.join(SKILL_DIR, "recipes", f"symi_{safe}.json")
    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    with open(out, "w", encoding="utf-8") as f:
        json.dump(payload, f, ensure_ascii=False, indent=2)
        f.write("\n")

    print(json.dumps({
        "success": True,
        "out": out,
        "id": symi["id"],
        "linePrimary": symi["linePrimary"],
        "lineSecondary": symi["lineSecondary"],
        "message": "Import this JSON in SYMI Ambient → 场景 → 导入"
    }, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    sys.exit(main())
