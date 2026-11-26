# 工作流与常用请求模板

> 由 SKILL.md 拆分而来，按需读取；不要整篇灌进上下文。

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
