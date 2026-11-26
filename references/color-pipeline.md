# 取色与配色推导（图片 / 多图 / 视频）

> 由 SKILL.md 拆分而来，按需读取；不要整篇灌进上下文。

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
