# WLED FX 仿真核心（host build）

在**没有真机 WLED** 的情况下，用 WLED 官方源码在 Linux 上重建效果渲染，
使 `probe_effect.py` / `verify_live.py` 的像素级判据可以照常运行。

## 为什么这样做

WLED 官方**没有提供任何仿真器**（`Aircoookie/WLED` 与 `wled/WLED` 均无 simulator 仓库）。
可选路径里：

| 方案 | 结论 |
|---|---|
| Docker + platformio + QEMU-ESP32 | 需下载数百 MB 工具链，6 GB 内存有 OOM 风险 |
| Python 重写 214 个效果的数学 | **结论不可信** —— 违背本项目"实测说话、不许猜"的信条 |
| **编译真实 WLED FX 源码** | ✅ 采用 |

关键前提：`wled00/FX_fcn.cpp`（113 KB，221 个效果的实现）对 ESP32 几乎零依赖：

```
esp_random 0 次   esp32-hal 0 次   driver/ 0 次
```

## 当前状态

| 单元 | 状态 | 说明 |
|---|---|---|
| `fastled_slim` | ✅ 编译通过 | CRGB / CRGBPalette16 / CHSV |
| `FX_fcn.cpp` | ✅ **编译通过，0 错误** | 221 个效果的数学核心 |
| `wled_math.cpp` | ✅ 编译通过 | sqrt32_bw 等 |
| `palettes.cpp` | ✅ 编译通过 | 调色板数据 |
| `colors.cpp` | ⏳ 待补 shim | 需要 `EspalexaDevice` 等 |
| `FX.cpp` | ⏳ 待补 shim | 同上（Segment 辅助方法） |
| harness + WebSocket 服务 | ⏳ 未开始 | 见下 |

构建：

```bash
./build.sh          # 只构建已验证的两个单元
./build.sh --all    # 尝试全部（未完成的会 [skip]）
```

## 关键设计

- **常量不手抄**：`src/wled.h` 直接 `#include "const.h"`（真实文件），
  所以 `FIXED_PALETTE_COUNT=72`、`WLED_CUSTOM_PALETTE_ID_BASE=200` 不会漂移。
- **影响渲染的数学抄真实实现**：`sqrt32_bw`（wled_math.cpp）、
  `scaledBri`（led.cpp）逐字照搬。
- **总线 / JSON / 存储全部惰性 stub**：FX 核心只用它们发布像素，
  而像素由我们自己截获，所以无需真实实现。

## 下一步

1. 补 `colors.cpp` / `FX.cpp` 所需 shim（`EspalexaDevice` 等 ESP 声明）
2. 写 harness：给定 `fx/sx/ix/pal/mi/of/led数`，逐帧渲染像素
3. 包成 fake WLED：`POST /json/state` + `ws://ip/ws` 的 `L\x01`+GRB 二进制帧
   —— 接口与真机一致，**现有探针脚本无需修改**
4. 跑全量扫描，产出 `references/fx-matrix.json`

## ⚠️ 仿真替代不了真机的事

仿真能判定「哪些效果丝滑 / 会写黑 / 空间是否相干」，但**替代不了真机抽检**：

- 真机的 gamma 校正 / 色彩管理，仿真里是近似的
- 真机的帧时序抖动（"丝滑"的真机决定因素）在 PC 上不存在
- LED 的 PWM 刷新与物理混色

因此矩阵条目应标注 `verified_on: sim`，拿到真机后**抽检 10–15 个**即可升级为
`verified_on: device`，无需全量重测。
