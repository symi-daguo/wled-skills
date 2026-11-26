# 设备能力自适应与现场口径

> 由 SKILL.md 拆分而来，按需读取；不要整篇灌进上下文。

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

## 白通道被 `col[0]` 钉住的效果（五色灯带必看）

`FX_fcn.cpp` `Segment::color_from_palette()` 末尾：

```cpp
CRGBW palcol = ColorFromPalette(_currentPalette, paletteIndex, pbri, blend);
palcol.w = W(color);   // color = getCurrentColor(mcol)
```

调用方传 `mcol=255` 时，`getCurrentColor(255)` 落到 `_currentColors[0]`，
于是**白通道恒等于段落 `col[0]` 的白分量**，与 `stage` / 位置**无关**。

两种后果，必须分清：

| 场景 | `col[0].W` 的作用 | 正确做法 |
|---|---|---|
| **静态氛围色**（`fx 161` + `sx=0`） | 就是「最终画面的白强度」，正合适 | 保留拆白算出的 W |
| **从暗到亮的动态效果**（`Sunrise` 104 等） | 是一层**恒定白底**：日出还没开始就已经满亮 | **W 全部置 0**，只让 RGB 参与渐变 |

实测证据（70 颗 WS2805）：`col[0].W=184` 时 t=3s 亮度 214（满亮）、全程不动；
W 置 0 后同一配方变成 41 → 175（x4.3），与 490 颗纯 RGB 的 38 → 176 几乎重合。

代码里由 `apply_recipe.W_PINNED_EFFECTS` 处理（目前 `{104}`）。
碰到别的效果「一开就满亮」，先怀疑这条，把 W 置 0 再试。

## IC（可寻址）还是 PWM（不可寻址）——按 `hw.led.ins[].type` 判

`const.h` 里类型编号是分段的，**不要看 `type` 的数字大小去猜能力**：

| 范围 | 含义 | 例 |
|---|---|---|
| `0 < type < 40` | **数字 / IC / 可寻址** | `22` WS281x、`32` WS2805 RGBCCT、`34` SM16825 |
| `type == 40` | 开关量（继电器），既不是 IC 也不是 PWM | `TYPE_ONOFF` |
| `41 ≤ type ≤ 47` | **模拟 / PWM / 整条同色** | `43` 3 通道 RGB、`45` RGB+WW+CW |

⚠️ `const.h:331` 有一行旧注释写着「`0b010 (dec. 32-47) analog (PWM)`」，**是错的** ——
`TYPE_WS2805 = 32` 明明是数字五色。权威判据是常量本身：`TYPE_ANALOG_MIN=41`、`TYPE_ANALOG_MAX=47`。

代码里由 `apply_recipe.device_caps()` 算出 `is_ic` / `is_pwm`；
`scene_palette.probe()` 也会直接打印 `IC(可寻址)` / `PWM(不可寻址)`。

### PWM 灯带要降级，不能假装逐颗

配方可以带一份 **`segments_pwm`**：

```json
{
  "segments":     [{"fx": 67, "pal": 5, "col": [[255,192,68],[255,77,109],[78,205,196]]}],
  "segments_pwm": [{"fx": 12, "pal": 5, "col": [[255,192,68],[255,77,109],[78,205,196]]}]
}
```

`apply_recipe` 探测到 PWM 时自动改用 `segments_pwm` 并告警；没给降级版就告警"逐颗会退化成整条同色"。
PWM 上能做的只有**缓慢换色**（`Fade`/`Gradient`），做不了"同一帧多种颜色"。

### 验收 "是不是真的逐颗"

| 指标 | IC 应 | PWM 应 |
|---|---|---|
| 同一帧颜色数（按 32 量化） | **≥3** | =1 |
| 相邻灯珠颜色不同占比 | **≥30%** | ≈0 |

实测 生日聚会：70 颗 6 色 / 64%；490 颗 17 色 / 66%。
