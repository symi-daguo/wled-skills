# 静态氛围色（场景名 → 三色渐变）

> 由 SKILL.md 拆分而来，按需读取；不要整篇灌进上下文。

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
