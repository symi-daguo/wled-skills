---
name: wled-skills
description: WLED 灯带技能集（WLED Skills）。按自然语言快速出光效：场景配色（14 组静态氛围色）、图片取色、视频取色、1 分钟动态配方、按名称复用、局域网多设备同步；自动识别设备能力（RGB / RGBW / RGBCCT 五色、任意灯珠数、PWM 或 IC 灯带）并据此出码，写入前备份、写入后回读像素验收。WLED light-strip skill set — build any lighting mode from natural language (scene palettes, image/video colour extraction, 1-minute dynamic recipes, recall by name, LAN multi-device sync), auto-adapting to RGB / RGBW / RGBCCT strips of any LED count, with backup-before-write and pixel-level verification.
version: 1.4.1
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
  - 氛围色
  - 场景配色
  - WLED
  - WLED 灯带
  - 灯带
  - 灯珠
  - 调光
  - 调色
  - 茉莉花
  - 花开
  - 日落
  - 海洋
  - 樱花
  - 灯光
  - 视频取色
  - 视频光配方
  - 多设备同步
  - 预设
  - light recipe
  - light effect
  - lighting scene
  - ambient color
  - scene palette
  - WLED control
  - WLED strip
  - led strip
  - scene lighting
  - video color
  - sync devices
  - jasmine
  - sunset
  - ocean
  - cherry blossom
  - preset
  - 灯带
  - 灯珠
  - 调色板
---

# WLED Skills v1.4.1

把一句话变成能上屏的光效，并**用设备自己的帧缓冲证明**它真的上了屏。
任意 WLED：RGB / RGBW / RGBCCT、任意灯珠数、PWM 或 IC 灯带。

## 一条命令搞定 90%（推荐上手方式）

统一入口 `scripts/wled.py`，动词 + 参数；`--dev <别名|IP>` 可写在动词前后。
别名在 `config.json` 的 `devices` 里（`{"strip70": "…", "strip490": "…"}`），
不用记 IP，也不会把内网地址写进对话。

| 你要做的事 | 命令 | ≈token |
|---|---|---|
| 有哪些灯带、各自什么能力 | `wled.py devices` | 0 |
| 现在什么状态 | `wled.py status`（`--json` 给机器读） | 0 |
| 开关 / 亮度 | `wled.py on` · `wled.py off` · `wled.py bri 180` | 0 |
| 给个颜色（中英文名或 `#hex`） | `wled.py color 暖白` · `wled.py color "#00B4D8"` | 0 |
| 氛围色（14 组场景） | `wled.py scene 热带海岸 --apply` | 0 |
| 换个效果（说人话也行） | `wled.py effect 呼吸` · `wled.py effect Pacifica` | 0 |
| 效果 + 调色板 + 速度 | `wled.py effect Pacifica --sx 60 --palette "* Color Gradient"` | 0 |
| 看本机有什么效果/调色板 | `wled.py effects 海` · `wled.py palettes 渐变` | 0 |
| 存到设备、下次一键触发 | `wled.py save "海边日落"` → `wled.py preset 3` / `wled.py recall "海边日落"` | 0 |
| 写前备份 / 收尾 | `wled.py backup` · `wled.py restore` | safety 1.5k |
| 做 1 分钟动态配方 | `apply_recipe.py <配方> --ip <ip>` + `references/effect-templates.md` | 4.4k |
| 设计顺序 / 物理速度推导 | `references/workflow.md` | 5.0k |
| 只有拿不准的才查文档 | `references/rules.md` · `intent-map.md` · `device-capability.md` · `scene-palettes.md` · `static-scenes.md` · `color-pipeline.md` · `recipe-format.md` | 0.4–2.1k |
| 某个字段的官方原文 | `references/wled-official/` **只查需要的单篇** | 0.4–25k |

**效果清单永远问设备**：`wled.py effects` / `GET /json/effects`（220 个，随固件版本变）。
不要读 25k token 的效果百科，也不要硬编码 id。

交付前过一遍 `references/done-checklist.md`（7 条必查，含"灯亮着且是客户要的样子"）。

## 三条硬规则（违反即返工）

1. **写前先备份**：`scripts/apply_recipe.py --backup --ip <ip>`（`/presets.json` + `/cfg.json`）。
2. **双灯带场景必须带 `--ip`**，否则会写到另一条灯带上；灯数与设备不符时脚本直接拒绝。
3. **测完熄灯 + 归还**：`scripts/scene_palette.py --off --ip <ip>`（熄灯 + `--release` 还原 `if.sync.recv`）。

完整写保护——会写数据的 GET、禁止残缺 `/settings` 表单、非幂等不重试、PIN 处理——
见 `references/device-safety.md`，**涉及写设备必读**。

## Token 预算（实测）

激活本文件 ≈ **2.0k**（拆分前是 ≈11.9k）。其余按需加载：

| 任务 | 额外加载 | 合计 |
|---|---|---|
| 一句话出场景色 | — | **2.0k** |
| 场景色 + 懂原理 | scene-palettes | 4.1k |
| 一条 1 分钟动态配方 | effect-templates + workflow | 11.4k |
| 查单个字段 | 对应单篇镜像 | 2.4k–27k |
| **下次一键触发** | **不加载任何东西** | **0** |

> **不要整目录读 `references/wled-official/`**：18 篇合计 ≈66k token，其中
> `features__effects.md` 单篇 ≈25k。它是查证用的字典，不是入门材料。
> 效果清单以设备自身的 `/json/effects` 为准（220 个，随固件版本变，硬编码必然过时）。

## 存到手机、下次一键触发（零 token 路径）

| 目标 | 做法 | 下次怎么触发 |
|---|---|---|
| **设备端长期可用（推荐）** | `save_preset.py --auto "<名字>"` → WLED 预设 1–250，掉电保留 | **WLED App 点预设 / 米家场景 / 物理按钮 —— 不经过 Agent，0 token** |
| 可分享、可版本管理 | 配方 JSON 落 `recipes/`，或 `export_recipe.py` 导出 | `recall_recipe.py "<名字>"`（先查 recipe 文件，再查设备预设） |

一句话：**Agent 只负责"创造"，重复触发交给设备端预设。** token 与延迟都最优。

## 环境与配置

- WLED 16.x，局域网 HTTP 可达。Python 3.10+（核心脚本**纯标准库**）；`Pillow` 用于图片分析；
  `ffmpeg`/`ffprobe` 用于视频截帧；`websocket-client` 用于像素回读。
- `cp config.example.json config.json` 后按需填（`wled.ip` 留空 = mDNS 自动发现）：

| 键 | 作用 |
|---|---|
| `wled.ip` | 设备 IP；留空自动发现 |
| `layout.type` / `layout.total_leds` | `tv_backlight` / `linear` / `ring` / `matrix`；灯数填 `0` = 跟随设备（推荐） |
| `safety.allow_write` | 写入门禁，由 `guard.py` 管理；**不要手改成 true 绕过办公模式** |
| `video.frames` / `video.mode` / `video.edge_band_ratio` | 截帧数 / `even`\|`scene` / 采样边框厚度（0.12 = 外侧 12%） |
| `sync.enabled` / `sync.group` | 主从同步开关与分组 |

- 内网 IP 一律不入库：`config.json` / `.wled_cache.json` / `backups/` 均已 gitignore。
- 像素验收：`scripts/verify_live.py`（WebSocket `{"lv":true}` 回读真实帧缓冲）。

## 文档地图

```text
SKILL.md                      ← 你正在读的（路由表，≈2.0k）
references/
├── device-safety.md          ← 写保护，写前必读
├── scene-palettes.md         ← 14 组场景配色 + 能力位出码
├── static-scenes.md          ← 静态渐变的渲染细节
├── effect-templates.md       ← 已验证模版 + 「不要这样写」
├── workflow.md               ← 设计顺序 / 常用请求 / 布局 / 错误处理
├── rules.md                  ← 硬规则 1–11
├── device-capability.md      ← 能力位、拆白、双灯带
├── color-pipeline.md         ← 图片 / 视频取色
├── recipe-format.md          ← 配方 JSON 与展示约定
└── wled-official/            ← 官方镜像 18 篇，只查单篇
```
