# WLED Skills

[简体中文](README.md) ｜ **English**

Agent skill set for **WLED** LED strips. Turns a sentence into a working light mode —
scene palettes, image/video colour extraction, one-minute dynamic recipes, recall by
name — and writes it to the strip on your LAN with **backup-before-write** and
**pixel-level verification**.

Works with any agent that can read a `SKILL.md` (WorkBuddy / 豆包 / Claude Code / any
SKILL.md-compatible runtime). No cloud service, no private binary, no vendor account.

## Why another WLED skill

Most WLED skills on GitHub are thin HTTP wrappers (`power on`, `color 255 0 0`,
`effect 9`). This one is built around three things they do not do:

| Capability | What it means |
|---|---|
| **Device capability adaptation** | Reads `info.leds.lc` and emits the right colour shape: 3 channels for RGB (WS281x / WS2815 / PWM), 4 channels + `cct` for RGBW / RGBCCT (e.g. WS2805 five-colour). White is split out as `w = min(R,G,B)` so the white die is actually driven instead of sitting at 0. |
| **Pixel-level verification** | Reads the real frame buffer over the WebSocket Peek stream (`{"lv":true}`) instead of trusting `/json/state`. Fields lie: with `lor>0` the API still reports `lm="UDP"` while every pushed frame is discarded. |
| **Verified effect library** | Every template carries the measurement that let it in, and disproved writing styles go to a "不要这样写" list. Includes the Shimmer `sx=0` static-gradient trick, the `pal 4 = (ter, sec, prim)` slot order, and the scene-change reset that teleports a crest. |

## Token budget (measured)

Activating the skill loads `SKILL.md` only — a 147-line router at ≈1.9k tokens
(down from 978 lines / ≈11.9k). Everything else is loaded on demand:

| Task | Extra load | ≈tokens |
|---|---|---|
| Activate | `SKILL.md` | **1.9k** |
| One-line scene colour | script-internal table | 1.9k |
| One-minute dynamic recipe | `effect-templates.md` + `workflow.md` | 11.3k |
| Look up one JSON field | one mirrored page | 2.3k–27k |
| **Re-trigger later** | **nothing** (device-side preset) | **0** |

Rules of thumb: query the device for the effect list (`GET /json/effects`) instead of
reading the 25k-token effects encyclopaedia; open `references/wled-official/` one page
at a time (18 pages ≈66k); and let the device own repeat triggers — save a preset and
fire it from the WLED app, Mi Home scene, or a physical button.

## Requirements

- A WLED device (16.x tested) reachable over HTTP on the same LAN
- Python 3.10+ (stdlib only for the core scripts; `Pillow` for image analysis, `ffmpeg` for video)
- `websocket-client` for pixel verification

## Install

```bash
# 1) as a local skill (WorkBuddy / Claude Code style)
ln -sfn "$PWD" ~/.workbuddy/skills/wled-skills

# 2) or clone anywhere and point your agent at SKILL.md
git clone https://github.com/symi-daguo/wled-skills.git

# 3) or import from GitHub URL if your agent supports it
```

Copy `config.example.json` to `config.json` and fill in your device IP (or leave it
empty and let `scripts/discover_wled.py` find it).

## Quick start

```bash
# what devices are on the LAN, and what are they capable of?
python3 scripts/scene_palette.py --probe --ip <strip-ip>

# a scene name -> a recipe, printed as JSON
python3 scripts/scene_palette.py "热带海岸"

# natural language works, in Chinese or English
python3 scripts/scene_palette.py "春天樱花那种粉" --apply --ip <strip-ip>
python3 scripts/scene_palette.py "warm white"    --apply --ip <strip-ip>

# dynamic 1-minute recipe from an image / video
python3 scripts/analyze_image.py photo.jpg
python3 scripts/apply_recipe.py recipes/<name>.json --ip <strip-ip>

# teardown: light off + give the sync settings back (never skip)
python3 scripts/scene_palette.py --off --ip <strip-ip>
```

## What you can build

| Need | Entry point |
|---|---|
| 14 curated ambient scenes (sunset, cherry blossom, glacier, aurora…) | `scripts/scene_palette.py --list` |
| Any single colour by name, zh or en | `scripts/scene_palette.py "暖白"` |
| Colours extracted from a photo | `scripts/analyze_image.py` + `scripts/apply_recipe.py` |
| Colours extracted from a video's key frames | `scripts/analyze_video.py` + `scripts/apply_video_recipe.py` |
| Smooth 1-minute evolving effect (verified templates) | `references/effect-templates.md` |
| Recall a saved recipe by the name the user gave it | `scripts/recall_recipe.py` |
| Master/slave sync across several strips | `scripts/sync_devices.py --ips a,b` |
| Save / restore a device snapshot | `scripts/apply_recipe.py --backup` |

## Safety

WLED has **no accounts** — anyone on the LAN can drive the device, and a partial
`POST /settings/*` blanks every field you omit. So this skill follows a write-protection
policy documented in [`references/device-safety.md`](references/device-safety.md):

- `--backup` snapshots `/presets.json` + `/cfg.json` before the first write
- some `GET`s actually write (`/reset`, `/settings/lock`, `/update?revert`, `/win&…`)
- never retry non-idempotent calls (`"on":"t"`, `"np":true`, `"psave"`)
- never POST a partial settings form; use `POST /json/cfg` and read back to compare
- never expose a WLED device to the internet

## Layout

```text
wled-skills/
├── SKILL.md                  # the skill contract (Chinese primary, English summary)
├── README.md / README.en.md  # this file, in Chinese / English
├── references/
│   ├── device-safety.md      # write-protection rules — read before any write
│   ├── scene-palettes.md     # 14 scenes + capability-aware encoding
│   ├── effect-templates.md   # verified templates + "don't write it this way"
│   ├── recipe-design-methodology.md
│   └── wled-official/        # mirrored official docs
├── scripts/                  # discovery, analysis, apply, verify, backup
└── recipes/                  # example recipes (portable JSON)
```

## License

MIT — see [LICENSE](LICENSE).
