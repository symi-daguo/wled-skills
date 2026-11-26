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
