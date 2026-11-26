# 硬规则（Rule 1–11）

> 由 SKILL.md 拆分而来，按需读取；不要整篇灌进上下文。

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
