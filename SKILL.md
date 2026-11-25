---
name: wled-light-recipe
description: WLED智能灯带光配方设计师。支持图片取色与视频取色（截关键帧、取画面四周颜色映射到灯带四段）、生成平顺丝滑且随时间演变的动态灯光效果、按名称复用已创建的光配方，并同步到局域网内多台 WLED（主从同一效果）。写入局域网设备需在豆包办公模式下解锁。当用户需要基于场景的灯光效果（如茉莉花、日落、海洋、某段视频）、WLED控制、光配方、多设备同步时调用此技能。
version: 1.0.8
author: symi-daguo
category: smart-home
permissions:
  - network
  - read
  - execute
triggers:
  - 灯光效果
  - 光配方
  - 灯光场景
  - WLED
  - 茉莉花
  - 花开
  - 日落
  - 海洋
  - 灯光
  - 视频取色
  - 视频光配方
  - 多设备同步
  - light recipe
  - light effect
  - WLED control
  - scene lighting
  - video color
  - sync devices
  - jasmine
  - sunset
  - ocean
  - 灯带
  - 灯珠
  - 调色板
---

# WLED Light Recipe Skill v1.0.5

Design smooth, continuously animating light recipes for WLED LED strips.
Turn "jasmine blooms" or "this video's colour mood" into a gentle flowing
gradient on your LED strip, and make every device on the LAN show it together.

## What This Skill Does

Transforms a scene description (like "jasmine blooms" or "sunset over ocean"),
or a **video the user points at**, into a smooth WLED light recipe by:

1. Searching 2-4 relevant images with built-in web search — or, for a video,
   downloading it and cutting key frames at the colour transitions
2. Analyzing dominant colors from ALL images / frames
3. Merging colors into a harmonious palette
4. Selecting a verified SMOOTH effect (Fade/Dissolve/Breathe/Forest/Sunset/Aurora)
5. Applying with SLOW speed (SX=70) and LOW intensity (IX≤120) for non-flashing animation
6. Saving as a WLED preset for reuse — recallable later **by the name the user chose**
7. Exporting a portable JSON recipe file for sharing

### Video recipes (v1.0.5)

When the user asks for "创建一个 X 效果的光配方" and answers with a **video**,
the skill takes the colour **from the four edges of each key frame** and maps
them onto the four segments of a TV backlight:

```
frame edge   ->  segment          example (top/right/bottom/left)
top          ->  seg 0  (0-20)    橙 -> 青 -> 柠檬绿
right        ->  seg 1  (20-35)   品红 -> 天蓝 -> 翡翠绿
bottom       ->  seg 2  (35-55)   深蓝 -> 藏蓝 -> 墨绿
left         ->  seg 3  (55-70)   金 -> 蓝绿 -> 黄绿
```

Each segment keeps the frame-by-frame sequence of its own edge, so the strip
**evolves over time** (three colours per segment driven by a slow smooth FX)
instead of freezing on one static picture — that is what makes the LED output
read as "the same" as the video, and it is what the sync packet carries to the
other devices.

### Two device-side gates you must know about

| Gate | What it does | Where |
|---|---|---|
| `lor` = 2 | Realtime override: our effect wins and incoming realtime frames (screen sync / SYMI Ambient) are **discarded**. Without it, a running stream takes the strip back within a second. Also required for preset saves to persist at all. | `apply_recipe.py` (auto) |
| `if.sync.send.en` **+** `state.udpn.send` | Master broadcast. Both must be true or the device never sends a sync packet. | `sync_devices.py --setup` |

Details and the measured evidence are in the sections below.

## Color Calculation Logic (How Images Become Light Recipes)

### Step 1: Image Download & Preprocessing
- Download image from URL using urllib
- Load with PIL (Pillow), resize to 150x150 max for speed
- Convert to RGB mode (remove alpha channel)

### Step 2: Color Quantization (PIL quantize method)
- Use PIL `Image.quantize(colors=N)` to reduce colors to top N (default 5)
- This groups similar pixels and returns the most representative colors
- Fallback: 4x4x4 RGB histogram (64 buckets) if quantize fails

### Step 3: HSV Conversion & Filtering
Each extracted RGB color is converted to HSV and filtered:
- `min_brightness=20`: Skip colors with V<20 (too dark, invisible on LED strip)
- `min_saturation=30`: Colors with S<30 (gray/white) are separated into `gray_colors`
- If saturated colors < 3: gray_colors are added back as fallback (preserves white flowers, clouds, snow)

### Step 4: Color Ratio Calculation
- Each color's ratio = pixel_count / total_pixels
- Colors are sorted by ratio descending (most dominant first)
- Top N colors are selected and ratios normalized to sum=1.0

### Step 5: Multi-Image Color Merging
When analyzing 2-4 images, colors from ALL images are merged:
1. Collect all colors from all images into one pool
2. Sort by ratio descending
3. Pick top 3 DISTINCT colors (hue difference > 30 degrees, RGB difference > 50)
4. This ensures color variety (e.g., green from leaves + yellow from flower center + white from petals)

### Step 6: Theme Detection & FX Selection
Dominant theme is determined by averaging hue values weighted by ratio:
- Hue 0-60°, 330-360° = "warm" → FX 13 (Sunset, smooth warm gradient)
- Hue 60-180° = "nature" → FX 12 (Fade, smoothest color crossfade)
- Hue 180-270° = "cool" → FX 50 (Aurora, smooth northern lights)
- Hue 270-330° = "party" → FX 12 (Fade, default smooth)

### Step 7: Parameter Tuning
- SX (Speed) = 70: Slow, gentle movement (range 50-90, NEVER > 128)
- IX (Intensity) = 100: Subtle, balanced (range 80-120, NEVER > 150)
- BRI (Brightness) = 180: Comfortable (range 150-200)
- These values are verified on real WLED 17.0.0-devV5 to produce smooth, non-flashing animation

## Static Ambient Scenes (scene name -> palette, fastest path)

When the user asks for a **static mood colour** ("来个热带海岸"、"樱花那种粉"、"生日温馨点的灯"),
do NOT go through image search or a 1-minute playlist. This is a **static ambient scene**,
not a dynamic recipe:

```bash
python3 scripts/scene_palette.py --list                      # 14 curated scenes
python3 scripts/scene_palette.py "热带海岸"                   # recipe JSON on stdout
python3 scripts/scene_palette.py "海边日落" --emit state      # directly POST-able /json/state
python3 scripts/scene_palette.py "樱花" --bri 180 --apply      # build + push to the LAN device
```

Read `references/scene-palettes.md` before changing anything in that path. Three facts it
records that are easy to get wrong:

1. **`pal 4` = `(ter, sec, prim)`** — the gradient runs `col[2] -> col[1] -> col[0]`, so the
   START colour goes in `col[2]`, not `col[0]`.
2. **`fx 161` (Shimmer) with `sx=0` is the only truly static full-strip gradient.** `FX.cpp`
   locks `position = (SEGLEN << 7)` when speed is 0, and it samples
   `color_from_palette(i*255/SEGLEN)`, i.e. the whole palette across the strip. `Gradient`
   (fx 46) keeps advancing its phase even at `sx=0`.
3. **`c1` must be 255** (`radius ≈ 2*SEGLEN`) so the whole strip stays inside the band and
   the ends only blend ~25% toward `col[1]`.

`scene_palette.py` emits a **flat recipe with an explicit `segments` list**, so it flows
through the same `apply_recipe.py` gate as every other recipe (clamp, stale-segment
cleanup, `clamp_warnings`, `/presets.json` verification). Do not write `/json/state`
directly when `--apply` is available.

## Dynamic Animation Strategy (1-Minute Loop)

**Read `references/recipe-design-methodology.md` FIRST.** It holds the customer's
hard rules and the mandatory design order (real reference photo -> animation
first/last frame -> physically derived speed -> four-edge spatial layout -> probe
measurement -> recipe). The sections below are the shortcut version.

**Then copy a skeleton from `references/effect-templates.md` - do NOT invent from
scratch.** That file is the verified template library ("参考模版"): every entry
carries the measured evidence that let it in, and every disproved writing style
goes into its "作废模版" list at the bottom. New animation requests MUST start from
the closest template (directional natural phenomenon / smooth full-ring / sea
base layer) rather than re-deriving parameters by trial and error.

### Two hard constraints that cause most rework (learned the hard way)

1. **Every scene-changing playlist must keep the animated segment's `fx`, `mi`,
   `of`, geometry, `ix` and `c1` CONSTANT.** Changing `ix`/`c1` changes Shimmer's
   `inputstate` and teleports the crest back to the start; changing `mi` or `fx`
   triggers `markForReset()`. Measured cost of ignoring this: a single-frame
   mirrored pair of jumps of 349 at every scene change. Only `sx` / `bri` / `col`
   / `pal` are safe to vary per scene.
2. **POST `/json/state` needs `Content-Type: application/json`** on this firmware,
   or the whole body is silently ignored (no error, no effect). `/json/presets`
   GET is not implemented (`ERR_NOT_IMPL`) - read `http://<ip>/presets.json`.

3. **"Applied successfully" is not "applied as written".** `apply_recipe.py` has an
   anti-flash clamp on `sx`/`ix`, and the same slider id means different things in
   different effects (`ix` is *Interval/pause* for Shimmer, *Angle/spatial
   frequency* for Pacifica, *Intensity* for most others). It is now effect-aware
   and reports every coercion it makes in `clamp_warnings` (empty = verbatim).
   Always re-read `/presets.json` and compare `sx`/`ix` against the recipe file;
   for Shimmer/Pacifica the anti-flash limits do not apply — neither can strobe.

### Parameter policy — the generic SX/IX limits have effect-aware exceptions

The generic anti-flash rule above (`SX 50-90`, `IX 80-120`) still governs ordinary
effects. It must NOT be applied blindly to `fx 161` (Shimmer) or `fx 101`
(Pacifica): for Shimmer, `sx` is a traversal time (higher = faster) and `ix` is a
post-sweep pause (`ix*236` ms, so `ix=80` forces an 18.9 s dead gap), and for
Pacifica `ix` is a spatial angle. Their ranges live in `_EFFECT_AXIS_EXEMPT`.

### Order of operations (do not reorder)
1. **Find a real reference** (free-licence photo/video of the actual scene) - never colour from imagination.
2. **Analyse it**: spatial structure (what is top/middle/bottom, left/right), real
   palette, and the animation's **first and last frame** of one cycle.
3. **Derive the speed from physics** (see "Physical speed" below) - never tune `sx` by feel.
4. **Map onto the device's actual geometry** - a home WLED is almost always a ring
   around a TV, so the four edges must compose **one coherent picture**.
5. **Probe the candidate effect** with `scripts/probe_effect.py` (real pixel
   read-back) before it goes into a recipe.
6. **Apply, then verify** with `scripts/verify_live.py`. It must report
   `black_pixels == 0`, `leds_never_lit == []`, `segments_cover_all_leds`,
   `playlist_advanced`, and **`smooth_motion`** - the last one is three
   measurements together: frame-to-frame per-LED delta mean <= 8, p95 <= 30, and
   **frozen-frame ratio <= 5%**. The frozen-frame ratio is the one that catches
   integer-phase effects (Gradient, fx 46 measured 36.1% frozen = "一抖一抖"),
   because a frozen frame contributes 0 to the mean and hides the defect.

### Physical speed (never guess `sx`)
A real wind swell has a period of **8-14 s**; take `T = 10 s`. The wave runs from
the horizon to the shore in about `T/2`, and the horizon->shore span is about half
of the bezel loop, so a full lap of the loop should take `T`:

```
target_px_per_second = N / T          # N = LED count
N = 70, T = 10  ->  7 LEDs/second  ->  one lap in 10 s
```

Measure the real value with `probe_effect.py` (it reports `px_per_second` and
`lap_seconds`, plus a `verdict` of `ok` / `too-fast` / `too-slow`) and only then
choose `sx`. "海滩运动没你测试的这么快" came from skipping exactly this step.

### When to use a playlist (and when not to)
Use a **playlist** whenever the request is for "约 1 分钟不同的光效 / 循环播放":
- `dur` and `transition` are in **tenths of a second**
- `repeat: 0` = endless loop (`playlist.cpp`); `repeat >= 1` stops by itself
- `transition` is a crossfade, so there is **no black frame** between scenes
- scenes are saved as individual presets first, then verified in `/presets.json`
- `apply_recipe.py` routes any recipe containing `"scenes"` to this path

A single-effect recipe is still fine when one effect genuinely *is* the named
motion (e.g. a gentle Fade for "呼吸"). Do not force a playlist where a single
sustained effect reads better.

### Reference: how FX 12 (Fade) is timed
```cpp
unsigned counter = (strip.now * ((SEGMENT.speed >> 3) + 10));
uint8_t lum = triwave16(counter) >> 8;   // period 65536
```
At `sx = 70`: `counter = now_ms * 18`, one fade cycle = `65536 / 18 ≈ 3.6 s`.
Useful as a sanity check on WLED's time base, not as a formula for every effect —
each effect computes its own phase, so always **measure**.

## CRITICAL RULES (NEVER Violate)

### Rule 1: ONLY Use Smooth Effects
NEVER use flashing/strobe effects. The following are FORBIDDEN:
- FX 77 (Pacifica) - causes strobe at high speed
- FX 75 (Flow) - causes strobe
- FX 35 (Fire) - flickers like fire
- FX 33 (Fireworks) - strobe flashes
- FX 1 (Blink) - obvious strobe
- FX 17 (Twinkle) - random flashes
- FX 6 (Party) - rapid strobe
- FX 11 (Rainbow Runner) - fast strobe
- FX 19 (Splash) - abrupt color jumps

ONLY use these verified SMOOTH effect **names** (resolve ID from device `/json/effects`):
- Fade - smoothest color crossfade [DEFAULT]
- Dissolve - soft color melting
- Breathe - gentle breathing
- Sunrise - warm gradient
- Aurora - northern lights feel
- Pacifica / Lake - water (when available on device)

**Never hard-code FX IDs** (e.g. do not assume fx=10 is Forest — on WLED 16 it is Scan).
Use `scripts/resolve_fx.py` / `analyze_multi_images.pick_best_dynamic_fx`.

### Rule 2: NEVER Use High Speed/Intensity
- SX (Speed): MUST be 50-90 (slow, gentle)
- IX (Intensity): MUST be 80-120 (subtle, balanced)
- BRI (Brightness): MUST be 150-200 (comfortable)
- SX > 128 causes strobe/flashing - FORBIDDEN
- IX > 150 causes intense flashing - FORBIDDEN

### Rule 3: Multi-Image Fusion
Always analyze 2-4 images and MERGE colors. Single image = limited palette.
For a video, use 6-10 key frames and merge their four-edge colours the same way
(`scripts/color_edges.py` + `scripts/analyze_video.py`).

### Rule 4: Office Mode Is Required Before ANY Device Write
Every write path is gated. The skill refuses to touch the device until the user
has confirmed they are in Doubao's office mode, because that is the condition
the customer agreed to. The switch lives in `config.json`:

```bash
python3 scripts/guard.py --status                              # check
python3 scripts/guard.py --enable --confirm-office-mode        # unlock (explicit)
python3 scripts/guard.py --disable                             # lock again when done
```

- `config.safety.allow_write` defaults to **false**; without the explicit
  `--confirm-office-mode` flag the unlock is refused.
- Reads (`discover_wled.py`, `--dry-run`, `recall_recipe.py --list`,
  `sync_devices.py --status`) work while locked.
- Guarded writes: `apply_recipe`, `apply_video_recipe`, `save_preset`,
  `upload_palette`, `sync_devices`, `recall_recipe`.
- **Always `--disable` when the customer's request is finished** so the next
  session does not start unlocked.

### Rule 5: Leave the Device's Real Control Loop Alone
Two separate mechanisms decide who owns the LEDs. Do not confuse them:

- `{"live": false}` → exits realtime. Enough when the user's screen-sync
  software is OFF.
- `{"lor": 2}` → realtime override. **Required when the sync software is still
  running**: without it the stream re-takes the strip inside a second, and
  WLED also skips the whole main-loop block containing `handlePresets()`
  (`wled.cpp:131`), which silently throws preset saves away.

`apply_recipe.py` / `apply_video_recipe.py` handle this automatically
(`takeover="auto"`). When the override is engaged the result carries a warning,
because the customer's other sender is being dropped — tell the user and hand
control back with `python3 scripts/apply_recipe.py --release` when done.

### Rule 6: EVERY LED Must Be Used (and the stale-segment trap)

The customer will notice a partially-lit strip immediately. Two requirements:

1. The union of all active segments must cover `0..N` with no gaps and no
   overlaps that hide each other. Verify from `/json/state`: sum of live
   `len` must equal the device LED count, **and** `verify_live.py` must report
   `leds_never_lit: []`.
2. **Never send a `seg` array without clearing the tail.** WLED only updates the
   segments listed in the array (the array index *is* the segment id), and a
   leftover segment sits **above** the new ones (higher id = upper layer). A new
   1-segment recipe therefore gets hidden behind the 4 edge segments of an
   earlier one and only a sliver of the strip shows the new effect — exactly the
   "还是只有一小段距离，为什么不是所有灯珠" failure.

```python
# MUST go through this helper, never hand-build the seg array:
from apply_recipe import clear_stale_segments
seg = clear_stale_segments(ip, segments)   # appends {"id":N,"stop":0} for the tail
```

### Rule 7: NEVER Black at Any Moment

- Pick effects that **cannot** write black. Read the effect in `wled00/FX.cpp`
  and check for `fill(BLACK)`, `fadeToBlackBy`, or a brightness term that can
  reach 0 (e.g. FX 75 Lake: `lum = (index > wave3) ? index - wave3 : 0` -> black).
- Verified-safe examples on WLED 16: **Gradient (46)** (blends from the segment's
  own colour), **Colorwaves (67)** (min brightness ≈ 31), **Aurora (38)** (adds a
  non-black backlight when all three segment colours are set).
- A playlist `transition` is a crossfade, so scenes never black out between each other.
- Proof, not belief: `verify_live.py` must report `black_pixels: 0`.

### Rule 8: Natural Phenomena Must Be Spatially Coherent

A home WLED is a ring around a TV, so the four edges are **one picture**, not four
independent canvases. Planning four unrelated effects/colour sets per edge
produces something that looks like it is spinning in circles — the customer's
words were "哪里有转圈的海浪".

Do this instead:
- Decide what each edge **means** in the scene (for a beach: top = far sea,
  sides = the surf zone, bottom = the shore where the wave breaks and foam
  spreads).
- Give the motion **one direction** that matches the real phenomenon (a swell
  travels from the horizon to the shore - it does not orbit the screen).
- Keep the "geography" (where the sea is, where the sand is) fixed in space and
  let the wave travel **over** it, rather than rotating the geography itself.

### Rule 9: Probe Before Shipping; Speed Comes From Physics

- Never put an effect into a recipe because its *name* sounds right.
  Run `scripts/probe_effect.py` and look at the real read-back: black pixels,
  coverage, colour spread along the strip, travel direction, `px_per_second`.
- Never tune `sx` by feel. Derive the target from the real period of the
  phenomenon (`target_px_per_second = LED_count / period_seconds`) and compare.

### Rule 10: Use the Documented Mechanisms

WLED already solves most of what a recipe needs; prefer it over inventing:
- **`of` (segment offset)** - shifts an effect's start point along the segment and
  wraps: the native way to phase-shift a travelling wave.
- **`bm` (blend mode) + overlapping segments (v16)** - true layering, lower id =
  bottom. This is how to combine a water body and a wave crest on the same LEDs.
- **`grp` / `spc`** - group/spacing turns N physical LEDs into one virtual LED.
- **WebSocket `{"lv":true}`** - the documented "Peek" live LED stream, and the
  only honest way to verify what the strip is really rendering.
- **`pal` 2/3/4/5** - palettes built from the segment's own colours. `pal: 0`
  is effect-dependent and can fall back to a random/party palette, so it must not
  be used when the customer's colours have to be exact.

Official docs are mirrored in `references/wled-official/` (18 pages, fetched from
the `wled/WLED-Docs` repository). Read them instead of guessing.

### Rule 11: ALWAYS leave the strip off and the device released

Every verification spends real light in someone's room. Two mandatory teardown steps,
in this order, **before you report back**:

```bash
# 1) 熄灯（不要只靠 release；release 不关灯）
curl -s -X POST "http://<ip>/json/state" -H "Content-Type: application/json" -d '{"on":false}'
# 2) 归还被 apply 改过的 UDP 同步接收设置
python3 scripts/apply_recipe.py --release
```

Why `--release` is not optional: `apply_recipe.py` **turns off `if.sync.recv`
(bri/col/fx/pal) on the device** so a neighbouring WLED cannot overwrite the effect
you are testing. Leaving it off silently breaks the customer's master/slave sync.
`--release` restores the exact previous values from `scripts/.wled_net_backup.json`.

Also: `{"on":false}` is not enough on its own if the strip was in realtime mode — send
`{"live":false}` too, otherwise the last pushed frame stays latched.

## Device capability adaptation (applies to EVERY WLED, not just the lab)

The skill must drive **any** WLED strip: analog PWM, WS281x/WS2815, WS2805 RGBCCT,
SK6812 RGBW, SM16825. So **never key logic off an IC model name** — those ids are
firmware-version dependent. Key it off the capabilities the firmware reports:

| Source | Meaning |
|---|---|
| `GET /json/info` → `info.leds.lc` | capability bitmask: **`0x01` = RGB, `0x02` = W (white channel), `0x04` = CCT** |
| `info.leds.count` | real LED count — **the authority**, not `config.json` |
| `info.leds.rgbw` / `wv` / `cct` | deprecated convenience mirrors of the same bits |
| `cfg.hw.led.ins[].type` / `order` / `rgbwm` | diagnostic hints only (e.g. `32` = WS2805 RGBCCT) |

Source: `wled00/FX_fcn.cpp` `refreshLightCapabilities()` — `SEG_CAPABILITY_RGB/W/CCT`.

### What changes with the capabilities

| Device | `col` shape | `cct` | Notes |
|---|---|---|---|
| `lc=0x01` (WS281x / WS2815 / PWM RGB) | `[[R,G,B], …]` | omit | pure RGB, 3 channels |
| `lc&0x02` (RGBW / RGBCCT, e.g. WS2805) | `[[R,G,B,W], …]` | **send it** | see white split below |

**White split (required on 5-channel strips).** With `rgbwm: 0` (manual auto-white)
the host must fill the W channel itself, otherwise it stays 0 and the strip loses
half its brightness and looks washed out.

```
w  = min(R, G, B)          # the achromatic part
r' = R - w, g' = G - w, b' = B - w
cct = clamp(127 + (R - B) / 2, 0, 255)   # neutral 127; redder = warmer (higher)
```

Verified example (cream `#FFF5E6` on a WS2805): `col[0] = [25, 15, 0, 230]`, `cct=140`
— hue preserved, 230 of white actually driving the white die.

> The WebSocket Peek stream (`{"lv":true}`) reports **RGB only**, but WLED adds the
> white channel into that RGB, so a correct 4-channel colour still reads back bright
> and non-black. Do not "fix" a low `col[0]` R value on an RGBW strip — that is the
> split, not a bug.

### Multi-strip labs: always pin `--ip`

`apply_recipe.py` resolves a single device by discovery. On a LAN with more than one
strip that is ambiguous, so every write must be pinned:

```bash
python3 scripts/apply_recipe.py recipe.json --ip <strip-ip>
python3 scripts/scene_palette.py "热带海岸" --ip <strip-ip> --apply
```

`scene_palette.py` refuses when the recipe's LED count ≠ the device's real count, and
`apply_recipe.build_segments()` now treats `info.leds.count` as the authority
(a mismatch with `config.layout.total_leds` is reported in `clamp_warnings`, no longer
silently used).

## LAN device facts (do not mix them up)

The lab currently has **two** WLED strips, and they are *different products*:

| Strip | LED count | Capability | Which recipes apply |
|---|---|---|---|
| 70 LEDs (`wled-772980`) | 70 | **RGB+W+CCT** (`lc=0x07`, WS2805 RGBCCT) | the default `layout.total_leds`; every template in `references/` assumes this |
| 490 LEDs | 490 | **RGB only** (`lc=0x01`, WS2815 driven as WS281x) | `--leds` follows the device automatically; **do not** run the 70-LED templates against it |

Rules that follow from that:

1. **Resolve the target before writing** — `python3 scripts/discover_wled.py` prints the
   IP it will use. If the recipe's LED count ≠ the device's `info.leds.count`, stop:
   you are pointing at the wrong strip.
2. **Never assume a stale IP.** Discovered IPs live in `scripts/.wled_cache.json` (gitignored);
   `config.json` is local-only and gitignored too — no internal IP ever goes into the repo.
3. **Multi-device sync is opt-in.** `sync_devices.py --ips a,b` is the only thing that
   should touch `if.sync.*`; don't hand-write it while testing a single strip.

## Prerequisites

- A WLED device on the local network (auto-discovered via mDNS or configured in config.json)
- Python 3.8+ with Pillow (pip install Pillow) for image analysis
- ffmpeg + ffprobe on PATH for video key-frame extraction
- Doubao built-in tools: web search, code execution

## Configuration

Before first use, edit `config.json` in the skill directory:
- `wled.ip`: WLED device IP (leave empty for mDNS auto-discovery)
- `layout.type`: LED strip layout - "tv_backlight", "linear", "ring", or "matrix" (default: "linear")
- `layout.total_leds`: Total LED count (set to 0 to auto-detect from device)
- `safety.allow_write`: must be true before any device write (see Rule 4).
  Managed by `guard.py`; do not hand-edit it to bypass the office-mode step.
- `video.frames` / `video.mode`: key-frame count and extraction mode
  (`even` | `scene`); `video.edge_band_ratio`: how thick the sampled border is
  (0.12 = outer 12% of each side).
- `sync.enabled` / `sync.group`: default master/slave sync group.

## Complete Workflow (STANDARD TEMPLATE)

When a user requests a light recipe, follow these steps EXACTLY.
**Any AI agent can follow this template to produce smooth light effects.**

### Step 0: Unlock Writes (Office Mode) — BEFORE anything that touches the device

```bash
python3 scripts/guard.py --status
# if locked, ask the user to confirm they are in 办公模式, then:
python3 scripts/guard.py --enable --confirm-office-mode
```

Analysis-only steps still work while locked, so it is fine to prepare the recipe
first. Remember to `--disable` when the request is finished.

### Step 1: Discover WLED Device

```bash
python3 scripts/discover_wled.py
```

- Note the `ip` field from JSON output, use as {WLED_IP} in later steps
- If failed: ask user "请告诉我WLED灯带的IP地址"
- Discovery health-checks the cached IP before trusting it. Never read
  `scripts/.wled_cache.json` directly — a stale cache once sent writes to an
  offline device while the real one had moved to a new lease.

### Step 2: Search for 2-4 Real High-Quality Landscape Images (CRITICAL)

Use Doubao's built-in web search to find REAL photographs for the scene.

**MANDATORY Image Requirements:**
1. **REAL photographs only** - NO illustrations, NO cartoons, NO AI-generated art, NO clipart
2. **Landscape orientation** - width > height (portrait images will be REJECTED)
3. **High resolution** - minimum 1280px width (1920x1080 or 4K preferred)
4. **2-4 images** - need multiple images for rich color variety
5. **Diverse content** - different angles, lighting, backgrounds of the same scene

**Search Query Template:**
```
{scene} photo high quality landscape 4K
```

Examples:
- "jasmine flower bloom photo high quality landscape 4K"
- "sunset ocean photo high quality landscape 4K"
- "cherry blossom photo high quality landscape 4K"

**How to Get Image URLs:**
1. Use Doubao web search with the query above
2. From search results, find image URLs (https://...jpg/.png)
3. **ONLY use commercially-free sources** (Pexels License / Unsplash License — both free for commercial use, no attribution required): **pexels.com**, **unsplash.com**. Never use wallpaper sites (e.g. wallpaperaccess.com) or any source lacking a clear free-commercial license; using copyrighted images in a commercial product is a legal risk.
4. Verify each URL is a direct image link (ends with .jpg, .png, or returns image content)
5. The script will automatically validate: landscape orientation + min 1280px width
6. Portrait/low-res images are auto-skipped, so collect 4-6 URLs to ensure 2-3 valid ones

**IMPORTANT:** Do NOT use locally generated or synthetic images. The skill validates image quality
and will reject images that don't meet landscape + HD requirements.

### Step 3: Apply Dynamic Recipe (ONE COMMAND - RECOMMENDED)

```bash
python3 scripts/apply_dynamic_recipe.py <url1> <url2> <url3> --scene "Scene Name" --save-preset --show-images --device {ADB_DEVICE}
```

This single command does EVERYTHING:
1. Downloads and validates all images (landscape + HD only, auto-skip invalid)
2. Analyzes colors from valid images using PIL quantize + white detection
3. Merges colors into 3-color palette (preserves white flowers, clouds, etc.)
4. Selects best SMOOTH effect (Fade/Dissolve/Breathe/Forest/Sunset/Aurora)
5. Applies with SX=70, IX=100 (slow, smooth, non-flashing)
6. Saves as WLED preset for reuse
7. **NEW**: Pushes source images to Android device screen via ADB (if --show-images)

**Parameters:**
- `--show-images`: Push source images to screen so customer sees where colors came from
- `--device 192.168.2.43:5555`: ADB device address (Android TV, amplifier with screen)
- `--duration 30`: How long to display images (seconds, default 30)
- `--local`: If no ADB device, open images on local computer instead
- `--save-preset`: Save as WLED preset for later recall

The light effect will start playing immediately with smooth, continuous animation.
Source images will display on screen simultaneously so customer understands the color origin.

### Step 3b: Video Recipe (when the user points at a video)

```bash
python3 scripts/apply_video_recipe.py <video_file_or_url> \
  --scene "Scene Name" --count 8 --mode even
```

One command does the whole video pipeline:

1. Extracts key frames with ffmpeg (drop `assets/.wled_asset*` / temp frames when done)
   - `--mode even`: evenly spaced, good for a trailer/loop with gradual colour drift
   - `--mode scene`: ffmpeg scene-change detection, good for hard cuts; falls back
     to `even` automatically when the video has no detectable scene changes
   - near-black frames are discarded so a fade-out does not inject black into the palette
2. Reads the colour of each frame's **four edges** (`color_edges.py`, outer
   `video.edge_band_ratio` of each side) and keeps a per-edge time sequence
3. Collapses the frames into a merged palette plus per-edge sequences
4. Writes 4 segments whose `col` arrays are the per-edge sequences, so each edge
   cycles its own colours over time with a slow smooth FX
5. Saves a named preset and persists `recipes/<scene>_<timestamp>.json`

Useful flags:

| Flag | Meaning |
|---|---|
| `--count N` | number of key frames (6-10 is the sweet spot) |
| `--mode even\|scene` | frame selection strategy |
| `--frames a.jpg b.jpg ...` | skip ffmpeg and use your own frames |
| `--takeover auto\|on\|off` | how hard to fight for the strip — see Rule 5 |
| `--band 0.12` | edge band thickness |
| `--no-preset` | don't touch the device's preset table |
| `--sync` | also (re)configure master/slave sync |
| `--palette-mode col` | skip the custom palette upload (recommended default) |
| `--json` | machine-readable output only |

**Report the measured result, not just "done":** the JSON contains
`edge_colors_hex`, `palette_colors_hex`, `verified.fps`, `preset_verified` and
`realtime`. Quote them — `fps ≈ 40+` is the proof that our effect engine really
owns the strip rather than an external stream.

### Step 3c: Multi-Device Sync (all devices show the same thing)

```bash
python3 scripts/sync_devices.py --setup            # first device becomes master
python3 scripts/sync_devices.py --setup --ips 192.168.2.9,192.168.2.10 --group 1
python3 scripts/sync_devices.py --selftest         # prove the broadcast really happens
```

- Only the **master** needs the recipe written to it; WLED broadcasts on every
  change and the slaves follow (fx, palette, colours, **and segment layout**).
- `--setup` writes both gates (`if.sync.send.en` plus the runtime
  `state.udpn.send`) and reads them back; a single-device LAN is fine to
  configure — add the second device later and re-run `--setup`.
- Slaves get `recv.seg` + `recv.sb` enabled. This is **not** cosmetic: with
  `recv.seg` off (the WLED default) the receiver takes the header-only path and
  a four-segment TV recipe arrives as one flat full-strip colour.
- All devices must run the **same WLED version** — the packet carries the raw FX id.
- If a device has "Live segment only" (`if.live.mso`) on, `lor` is ignored there;
  `--status` reports it so you can tell the user to switch it off if they want
  the recipe to outrank a screen-sync stream.

### Step 4: Report to User

Tell the user:
- The scene name and recipe applied
- The merged colors extracted (show hex codes); for video recipes also the
  per-edge sequences (`top/right/bottom/left`)
- The effect name (FX 12 Fade, FX 50 Aurora, etc.)
- The speed (SX=70 slow) and intensity (IX≤120 subtle)
- The preset ID saved (for recall later) and that it can be recalled **by name**
- For video recipes: `fps` from the verified block, and whether a realtime
  override was engaged (and how to release it)
- Confirm: "效果平顺丝滑，持续循环播放，无闪烁"

## Common User Requests

### "Create a smooth light recipe for [scene]"

Run the full workflow above with 2-4 images. Example:
```bash
python3 scripts/apply_dynamic_recipe.py \
  "https://example.com/jasmine1.jpg" \
  "https://example.com/jasmine2.jpg" \
  "https://example.com/jasmine3.jpg" \
  --scene "Jasmine Blooms" --save-preset
```

### "Apply recipe [name]"

List saved recipes and apply:
```bash
python3 scripts/export_recipe.py --list
python3 scripts/import_recipe.py recipes/{file} --save-preset "{name}"
```

### "Stop the lights" / "Turn off"

```bash
python3 -c "
import urllib.request, json
req = urllib.request.Request('http://{WLED_IP}/json/state',
    data=json.dumps({'on': False, 'live': False}).encode(),
    headers={'Content-Type': 'application/json'})
urllib.request.urlopen(req, timeout=3)
print('Lights off')
"
```

### "Recall preset [ID]"

```bash
python3 -c "
import urllib.request, json
req = urllib.request.Request('http://{WLED_IP}/json/state',
    data=json.dumps({'ps': PRESET_ID}).encode(),
    headers={'Content-Type': 'application/json'})
urllib.request.urlopen(req, timeout=3)
print(f'Preset {PRESET_ID} recalled')
"
```

(Prefer `recall_recipe.py "<name>"` — it clears realtime first so the recall is
not silently swallowed, and it verifies `state.ps` afterwards.)

### "用刚才那个光配方" / "再用一次 [名称]" (recall BY NAME)

```bash
python3 scripts/recall_recipe.py --list            # show saved recipes
python3 scripts/recall_recipe.py "四边渐变测试"      # exact name, then substring
python3 scripts/recall_recipe.py "四边渐变测试" --preset-only
```

Lookup order: a local `recipes/*.json` whose **name** matches, otherwise a WLED
preset with that name. Recipe files are matched case-insensitively and by
substring, so "渐变" finds "四边渐变测试".

### "让所有灯带显示同一个效果" / "其它 WLED 也一起亮" (multi-device)

```bash
python3 scripts/sync_devices.py --status
python3 scripts/sync_devices.py --setup --ips {IP1},{IP2},{IP3} --group 1
python3 scripts/sync_devices.py --selftest
```

Then write the recipe to the **master only** — the others follow automatically.

### "我的屏幕同步软件还开着，会影响吗？" (realtime conflict)

This is the most common real-world snag, answer it with the measured behaviour:

| Situation | What works | Evidence |
|---|---|---|
| Screen-sync software OFF | `{"live": false}` is enough | `fps` 43 (recipe) |
| Screen-sync software ON | needs `{"lor": 2}` | stream alone `fps` 10-13; with override `fps` 43 |
| Same state written twice | **nothing is broadcast** | WLED notifies only on a real change (`led.cpp:95`) |

So: `apply_recipe.py` / `apply_video_recipe.py` default to `takeover=auto` — they
try `live:false` first, and only seize `lor=2` if the stream keeps winning. The
result then warns that the other sender is being dropped. Hand control back with:

```bash
python3 scripts/apply_recipe.py --release
```

Without the override, note that **preset saving silently fails** while a stream
runs, because WLED skips `handlePresets()` in realtime mode. `save_preset.py`
now exits realtime first and verifies the entry really landed in `/presets.json`
instead of trusting the `{"success":true}` response.

## Effect Selection Logic (Auto)

The `analyze_multi_images.py` auto-selects SMOOTH effects based on dominant theme:

| Dominant Theme | Selected FX (by name) | Description |
|---|---|---|
| warm (sunset/fire) | Sunrise / Fire 2012 / Fade | Smooth warm gradient |
| nature (forest/spring) | Fade / Dissolve / Aurora | Smoothest color crossfade |
| cool (ocean/sky) | Aurora / Pacifica / Fade | Cool water / northern lights |
| party (night/dream) | Fade / Dissolve / Rainbow | Default smooth |

IDs are resolved from the live module `GET /json/effects` — never hard-code.

## Layout-Aware Segments

The skill automatically creates segments based on `config.json` layout:

- **tv_backlight**: 4 segments (top, right, bottom, left). Image recipes rotate the
  merged palette across them; **video recipes give each segment its own colour
  sequence taken from the matching frame edge**, plus `edge_offsets`
  (`top:0, right:64, bottom:128, left:192`) so a shared palette spreads across the edges.
- **linear**: 1 segment covering all LEDs
- **ring**: 1 segment (treated as linear)
- **matrix**: 1 segment (user should configure 2D separately)

Segment order is fixed as `top, right, bottom, left` (`color_edges.EDGE_ORDER`) and
must match the LED ranges derived from `layout.edges`. Example on a 70-LED
backlight with `{top:20, right:15, bottom:20, left:15}`:

| seg | edge | LEDs |
|---|---|---|
| 0 | top | 0-20 |
| 1 | right | 20-35 |
| 2 | bottom | 35-55 |
| 3 | left | 55-70 |

Slaves only reproduce this layout when `recv.seg` + `recv.sb` are enabled — see
Step 3c.

## Error Handling

- **WLED not found**: Run `discover_wled.py --force` or ask user for IP
- **Image download fails**: Try next image URL from search results
- **UDP realtime active (live=true)**: Script auto-sends `{"live":false}` to take control
- **Preset save fails**: Recipe still applied, just not saved as preset

## Recipe File Format

All recipe files use a portable JSON format (`doubao-wled-recipe-v1`):

```json
{
  "name": "Jasmine Blooms",
  "description": "Jasmine flower colors",
  "version": "2.0",
  "format": "doubao-wled-recipe-v1",
  "colors": [[100,150,80], [179,219,149], [240,250,219]],
  "palette_data": [0,100,150,80, 85,179,219,149, 170,240,250,219, 255,254,229,100],
  "fx": 12,
  "sx": 70,
  "ix": 100,
  "bri": 180,
  "pal": 0,
  "theme": "nature",
  "rationale": "Merged 3 images, smooth FX 12 Fade for continuous animation"
}
```

## SYMI Ambient 首页展示规则（务必遵守）

与 Android App `SYMI-Ambient` 对齐：

| 类型 | 上行 | 下行 | 说明 |
|------|------|------|------|
| WLED 原生 Effect（App 精选 /「添加效果」） | 中文译名 | **官方英文**（与模组 `GET /json/effects` 字符串一致） | 不是「光配方」 |
| **本 skill 手工创建** | 固定文案 **光配方** | 客户录入的场景名 | 唯一称为「光配方」的条目 |

导出到 App 导入格式：

```bash
python3 scripts/export_symi_ambient.py recipes/jasmine.json --name "茉莉花盛开"
# → recipes/symi_茉莉花盛开.json
# 在 SYMI Ambient → 场景 → 导入
```

生成的 JSON 含 `kind=custom`、`linePrimary=光配方`、`lineSecondary=<客户名>`、`stateJson`（可直接下发 WLED）。

**禁止**把 skill 自定义场景伪装成 WLED 英文 Effect 名；**禁止**改写原生 Effect 的英文行。

## Directory Structure

```
wled-light-recipe/
├── SKILL.md                      # This file (skill instructions)
├── config.json                   # Device & layout configuration
├── config.example.json           # Config template with documentation
├── scripts/
│   ├── discover_wled.py          # mDNS discovery + health check; resolve_ip() is
│   │                             #   the single source of truth for the device IP
│   ├── guard.py                  # Office-mode write gate (config.safety.allow_write)
│   ├── analyze_image.py          # Single image color extraction + quality validation
│   ├── analyze_multi_images.py   # Multi-image analysis + color merging
│   ├── color_edges.py            # Four-edge colour sampling (top/right/bottom/left)
│   ├── video_frames.py           # ffmpeg key-frame extraction (even | scene)
│   ├── analyze_video.py          # Frames -> per-edge sequences -> video recipe
│   ├── apply_recipe.py           # Apply recipe to WLED (+ realtime takeover/release)
│   ├── apply_video_recipe.py     # ONE-SHOT video pipeline (RECOMMENDED for video)
│   ├── apply_dynamic_recipe.py   # ONE-SHOT image pipeline (RECOMMENDED for images)
│   ├── recall_recipe.py          # Recall a recipe BY NAME (file, else WLED preset)
│   ├── sync_devices.py           # Master/slave UDP sync setup + --selftest
│   ├── show_recipe_gallery.py    # Push images to screen via ADB
│   ├── build_playlist.py         # Playlist builder (advanced)
│   ├── save_preset.py            # Save current state as preset (realtime-safe, verified)
│   ├── upload_palette.py         # Upload custom palette
│   ├── export_recipe.py          # Export recipe to JSON file
│   ├── import_recipe.py          # Import & apply recipe
│   └── .wled_cache.json          # Cached WLED IP (auto-generated, never read directly)
├── references/
│   ├── wled_api.md               # WLED JSON API reference
│   └── effects_palettes.md       # Effects & palettes list
├── recipes/                      # Exported recipe files
└── assets/                       # Test images (real photos only)
```

**WLED palette ID space (16.x, if you touch `pal`):** built-in palettes are IDs
0-71, user custom palettes are IDs 72-200 where `/palette{N}.json` holds ID
`200 - N` and only becomes visible **after a reboot**. `upload_palette.py` and
`apply_video_recipe.py` compute this for you; `pal = file_index` is wrong and
would point at a built-in palette.

## Example Conversation

**User**: "我需要茉莉花盛开的灯光效果"

**Assistant**:
1. Discovers WLED device on LAN (e.g. 192.168.2.66)
2. Searches "jasmine flower bloom photo high quality" - gets 3 image URLs
3. Runs: `python3 scripts/apply_dynamic_recipe.py url1 url2 url3 --scene "Jasmine Blooms" --save-preset`
4. Script analyzes all 3 images, merges colors, selects FX 12 (Fade)
5. Applies with SX=70 (slow), IX=100 (subtle) - smooth, non-flashing
6. Saves as preset ID 100

**Response**: "茉莉花盛开光配方已应用！融合颜色：#8DA866 (绿), #B3DB95 (浅绿), #F0FADB (白)。效果：Fade (FX 12)，速度70（慢速），强度100（柔和）。效果平顺丝滑，持续循环播放，无闪烁。已保存为预设100。"

## Tips for Best Results

- Use 2-4 images for rich color variety (single image = limited palette)
- Use specific scene names ("jasmine blooms" vs just "flowers")
- For seasonal themes, add the season ("autumn forest" vs "forest")
- The skill auto-loops the effect until you change it
- To recall a saved recipe: "apply jasmine blooms recipe"
- To share: send the recipe JSON file from recipes/ directory

## Version History

- v1.0.5: 视频取色光配方（ffmpeg 关键帧 + 四边取色 → 四段各自的时间色序）、按名称复用
  （`recall_recipe.py`）、办公模式写入门禁（`guard.py`）、主从多设备同步
  （`sync_devices.py` + `--selftest`）、实时流抢占（`lor` 优先级与 `--release`）。
  同时修复四处静默失效：陈旧缓存导致写入指向离线设备、实时模式下预设保存被丢弃、
  同步广播缺少运行时开关 `udpn.send`、自定义调色板 ID 映射错误（`/palette{N}.json`
  → ID `200-N`）。
- v1.0.4: FX 按设备 `/json/effects` 英文名解析（`resolve_fx.py`），禁止硬编码 ID
- v1.0.3: SYMI Ambient 双行展示规则 + `export_symi_ambient.py`（光配方=skill 自建）
- v1.0.2: 新增图片质量验证(landscape+1280px)、ADB推送图片到屏幕、白色检测优化
- v1.0.1: 修复暴闪问题，仅用平滑FX(Fade/Dissolve/Breathe)，SX=70/IX=100，多图融合
- v1.0.0: 初始版本，单图分析，存在闪烁问题
