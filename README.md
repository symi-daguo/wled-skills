# WLED Skills

**简体中文** ｜ [English](README.en.md)

豆包 WLED 光配方技能 - 通过自然语言对话为 WLED 灯带生成动态灯光效果。

## 功能概述

对豆包说"我需要茉莉花盛开的灯光效果"，技能会自动：
1. 搜索茉莉花相关图片
2. 解析图片颜色分布
3. 生成自定义调色板上传到 WLED
4. 应用最佳匹配的灯光效果
5. 保存为 WLED 预设（循环播放直到更换）
6. 导出可分享的光配方 JSON 文件

### v1.0.5 新增：视频取色光配方

对豆包说"创建一个《XXX》那个效果的光配方"，并给它一段视频，技能会：
1. 用 ffmpeg 截取关键帧（均匀采样或场景切换检测）
2. 取**每一帧画面四边的颜色**，得到上/右/下/左四条时间色序
3. 把四边映射到灯带的四段（电视背光布局），每段保留自己随时间变化的颜色
4. 用缓慢平滑的效果循环播放，灯带会**跟着视频节奏演变**，不是一张静态画面
5. 存成预设并落盘配方文件，**下次直接说名字就能再次调用**

### v1.0.5 新增：局域网多设备同步

WLED 有主从同步。技能只需写入**主设备**，同组的其它灯带会自动跟随
（效果、调色板、颜色**以及分段布局**都一样）。

### v1.0.5 新增：办公模式写入门禁

所有写入设备的操作都有一道门禁：必须先确认处于豆包**办公模式**才能解锁。
这是默认关闭的开关，技能不会在未经确认的情况下改动你局域网的灯带。

### v1.0.5 新增：已验证的效果模版库 + 「丝滑」客观判据

- `references/effect-templates.md` —— **参考模版库**。每条模版都附带实测证据，
  新配方从最接近的模版改写，不再从零发明、不再靠试参数返工；被实测证伪的写法
  统一进文末「作废模版」。
- `scripts/verify_live.py` 新增 **`smooth_motion`** 判据（帧间均值 ≤ 8、p95 ≤ 30、
  **静止帧占比 ≤ 5%**）。静止帧占比是关键：整数相位效果（Gradient fx 46）画面会
  「冻结几百毫秒再跳一整颗灯」，实测静止帧占比 36.1%，而逐帧都在走的 Shimmer
  实测 0.0%。只看变化量均值会漏判，因为冻结帧记 0，反而把均值拉低。
- **换场景不重置动画**：Shimmer 的 `ix`/`c1` 一变就会把浪尖弹回起点，实测造成
  换场景处镜像成对的单帧跳变 349。配方里动画段的 `fx`/`mi`/`of`/几何/`ix`/`c1`
  必须全程恒定，只允许变 `sx`（速度）/`bri`/`col`/`pal`。

## 一条命令上手

```bash
python3 scripts/wled.py devices                  # 有哪些灯带、各自什么能力
python3 scripts/wled.py status --dev strip70     # 当前状态（--json 给机器读）
python3 scripts/wled.py scene 热带海岸 --apply --dev strip70
python3 scripts/wled.py color 暖白 --dev strip70
python3 scripts/wled.py effect 呼吸 --dev strip70                 # 说人话
python3 scripts/wled.py effect Pacifica --sx 60 --palette "* Color Gradient" --dev strip70
python3 scripts/wled.py save "海边日落" --dev strip70             # 存设备端预设
python3 scripts/wled.py restore --dev strip70                     # 熄灯 + 归还同步
```

- **设备别名**：`config.json` 的 `devices` 里写 `{"strip70": "…", "strip490": "…"}`，
  之后用别名代替 IP；`--dev` 写在动词前后都行。
- **说人话**：`effect 呼吸/海浪/星空/极光/烛光/电视…` 会翻译成设备上真实存在的效果名
  （映射见 `references/intent-map.md`，每个名字都在 220 个效果表里核对过）。
- **自描述**：`effects` / `palettes` 直接读设备，永远和固件版本一致，不用查文档。

## Token 预算（实测，不是估算）

技能激活时只加载 `SKILL.md`，其余一律按需：

| 场景 | 加载 | ≈token |
|---|---|---|
| 激活（每次都会） | `SKILL.md`（路由表） | **1.9k** |
| 一句话出场景色 | + 脚本内置表 | 1.9k |
| 做一条 1 分钟动态配方 | + `effect-templates.md` + `workflow.md` | 11.3k |
| 查某个 JSON 字段原文 | + 对应单篇镜像 | 2.3k–27k |
| **下次一键触发** | **不加载任何东西** | **0** |

取舍原则：

- **能问设备就不读文档**：效果清单走 `GET /json/effects`（220 个，随固件版本变），
  而不是把 25k token 的效果百科塞进上下文。
- **字典只查单篇**：`references/wled-official/` 18 篇合计 ≈66k token，仅供查证，不供通读。
- **创造归 Agent，重复触发归设备**：配方存成 WLED 预设（1–250，掉电保留）后，
  下次用 WLED App / 米家场景 / 物理按钮直接触发，**零 token、零延迟**。

## 安装方法

### 方法一：从 GitHub 仓库导入（推荐，手机版/桌面版通用）

仓库地址：`https://github.com/symi-daguo/wled-skills`

1. 打开豆包 App（手机版或桌面版）
2. 进入技能管理页面（通常在"我的"→"技能"或设置中）
3. 选择"从 GitHub 导入"或"添加技能"
4. 输入仓库地址：`https://github.com/symi-daguo/wled-skills`
5. 确认导入，豆包会自动读取根目录的 `SKILL.md` 加载技能
6. 技能名称显示为 `wled-light-recipe`，安装完成

### 方法二：本地导入

1. `git clone https://github.com/symi-daguo/wled-skills.git`
2. 打开豆包桌面端，切换到"办公任务"模式
3. 输入 `/创建技能` 或 `/install-skill`
4. 选择本地导入，指定 clone 下来的目录
5. 按提示完成安装

> 注意：「办公任务」模式不仅是安装技能的前提，也是**技能写入 WLED 设备的必要条件**。
> 技能里有一道写入门禁默认锁死，只有在办公模式下由用户确认才会打开。
> 关闭同步软件后灯带完全归技能控制；若你的屏幕同步软件仍在运行，技能会自动用
> `lor` 优先级接管，并在结果里提示"外部推送正被丢弃"，用完可用 `--release` 交还。

### 方法三：直接引用 Raw 链接

如果豆包支持 Raw 链接导入，可直接引用：
`https://raw.githubusercontent.com/symi-daguo/wled-skills/main/SKILL.md`

## 配置说明

安装后，编辑 `config.json` 配置你的 WLED 设备：

```json
{
  "wled": {
    "ip": "192.168.1.100",           // 你的 WLED IP（留空则自动发现）
    "mdns_name": "wled-xxxxxx",       // WLED 的 mDNS 名称
    "auto_discover": true             // 启用自动发现
  },
  "layout": {
    "type": "tv_backlight",           // 布局类型: tv_backlight/linear/ring/matrix
    "total_leds": 56,                 // 总灯珠数
    "edges": {                        // TV 背光四边灯珠数
      "top": 16, "bottom": 16, "left": 12, "right": 12
    },
    "start_corner": "top_left",       // 起始角落
    "direction": "cw"                 // 方向: cw(顺时针)/ccw(逆时针)
  },
  "safety": {
    "allow_write": false,             // 写入门禁，默认关闭；请用 guard.py 解锁
    "require_office_mode": true       // 必须办公模式才允许写入
  },
  "video": {
    "frames": 8,                      // 截取关键帧数量（6-10 较合适）
    "mode": "scene",                  // scene=场景切换检测 / even=均匀采样
    "edge_band_ratio": 0.12,          // 取色带宽：每条边外侧 12%
    "palette_id": 1                   // /palette{N}.json 文件索引（运行时 ID 见下）
  },
  "sync": {
    "enabled": false,                 // 是否配置主从同步
    "group": 1                        // 同步组号
  }
}
```

> 调色板编号提醒：WLED 16.x 里内置调色板占 ID 0-71，自定义调色板占 ID 72-200，
> 且 `/palette{N}.json` 对应运行时 ID `200-N`，需要**重启后**才可见。技能已自动换算；
> 视频配方默认直接用每段 `col` 写色，不依赖调色板，所以画面不受重启与否影响。

## 使用示例

### 创建灯光效果

| 说什么 | 效果 |
|---|---|
| "我需要茉莉花盛开的灯光效果" | 搜索茉莉花图片，生成绿色白色光配方 |
| "给我一个日落的灯光场景" | 搜索日落图片，生成橙红色暖光配方 |
| "樱花飘舞的灯光" | 搜索樱花图片，生成粉色光配方 |
| "海洋深处的灯光" | 搜索海洋图片，生成蓝色冷光配方 |
| "创建一个极光流动效果的光配方（附视频）" | 截关键帧取四边颜色，四段灯带各走各的时间色序 |

### 管理光配方

| 说什么 | 效果 |
|---|---|
| "列出所有光配方" | 显示已保存的光配方列表 |
| "应用茉莉花配方" | 重新应用已保存的配方 |
| "再用一次 四边渐变测试" | **按名称**复用（先查配方文件，再查 WLED 预设） |
| "关闭灯光" | 关闭 WLED 灯带 |
| "切换到预设 100" | 调用指定 ID 的预设 |

### 多设备同步

| 说什么 | 效果 |
|---|---|
| "让所有灯带一起显示这个效果" | 配置主从同步，只写主设备，其余自动跟随 |
| "检查同步状态" | 查看每台设备的角色、同步组、是否真的会发包 |
| "验证同步是否生效" | 在本机抓一次 UDP 广播，实测证明链路成立 |

## 技术要求

- WLED 固件 0.14+（推荐 0.15+ 或 Nightly）；**多设备同步要求所有设备版本一致**
- Python 3.8+ with Pillow（`pip install Pillow`）
- **ffmpeg / ffprobe**（视频取色功能需要；未安装时视频配方不可用）
- 豆包专业版（支持代码执行技能）
- WLED 设备与豆包在同一局域网

## 目录结构

```
wled-skills/
├── SKILL.md                    # 技能核心指令（豆包读取）
├── config.json                 # 设备与布局配置
├── scripts/                    # Python 脚本
│   ├── discover_wled.py        # mDNS 自动发现 + 健康检查（IP 唯一真源）
│   ├── guard.py                # 办公模式写入门禁
│   ├── analyze_image.py        # 图片颜色解析
│   ├── analyze_multi_images.py # 多图融合
│   ├── color_edges.py          # 画面四边取色
│   ├── video_frames.py         # ffmpeg 关键帧截取
│   ├── analyze_video.py        # 帧 → 四边色序 → 视频配方
│   ├── apply_video_recipe.py   # 视频一键管线
│   ├── apply_recipe.py         # 应用光配方（含实时模式接管/交还）
│   ├── recall_recipe.py        # 按名称复用光配方
│   ├── sync_devices.py         # 主从同步配置 + 广播自检
│   ├── save_preset.py          # 保存 WLED 预设（实时安全 + 落盘校验）
│   ├── upload_palette.py       # 上传自定义调色板
│   ├── export_recipe.py        # 导出光配方 JSON
│   └── import_recipe.py        # 导入并应用光配方
├── references/                 # 参考文档
│   ├── wled_api.md             # WLED JSON API 参考
│   └── effects_palettes.md     # 效果与调色板清单
├── recipes/                    # 导出的光配方文件
└── assets/                     # 测试资源
```

## 光配方文件格式

导出的光配方为便携式 JSON，可在任意 WLED 设备导入：

```json
{
  "name": "茉莉花盛开",
  "colors": [[82,107,79], [105,145,92], [53,80,46]],
  "fx": 10, "sx": 174, "ix": 159, "bri": 169,
  "theme": "nature",
  "rationale": "Green/yellow tones - nature/forest effect"
}
```

## 常见问题

**Q: 手机版豆包能用吗？**
A: 可以。技能使用 Python 标准库 + Pillow，兼容手机版豆包代码执行环境。如果手机版无法执行 Python，豆包会用视觉能力直接分析图片并输出光配方 JSON。

**Q: 多个 WLED 设备怎么办？**
A: 用主从同步。执行 `python3 scripts/sync_devices.py --setup --ips 设备1,设备2 --group 1`，
第一台作为主设备，其余自动编为从设备并开启段同步；之后**只需把光配方写给主设备**，
同组设备会自动跟随同一效果。用 `--selftest` 可在只有一台设备时也验证广播链路成立。
注意所有设备需运行相同 WLED 版本（同步包携带的是效果 ID 原值）。

**Q: 我的屏幕同步软件还开着，技能写的光配方能显示出来吗？**
A: 能。技能会先尝试退出实时模式；如果软件仍在推送，会自动用 `lor=2` 抢占优先级，
让灯带显示光配方（此时对方推送会被丢弃），并在结果里明确提示。用完执行
`python3 scripts/apply_recipe.py --release` 把控制权交还即可。
注意：在实时模式占用期间，WLED 会静默丢弃预设保存请求，技能已针对这一点做了
"先清实时再保存 + 回读校验"，不会再出现"提示保存成功其实没落盘"。

**Q: 视频取色是怎么取四周颜色的？**
A: 截取的每一张关键帧，按外侧 12%（可调 `video.edge_band_ratio`）切出上/右/下/左四条边带，
每条边带量化出主色；四边分别对应灯带的四段，且每段保留"帧→帧"的颜色顺序，
所以灯带会随时间在每段自己的颜色序列里平滑变化，而不是固定一种颜色。

**Q: 灯光效果会一直播放吗？**
A: 是的，效果会循环播放直到你更换其他配方或关闭灯光。保存为预设后，WLED 重启也会自动恢复。

**Q: 如何分享光配方给朋友？**
A: 将 `recipes/` 目录下的 JSON 文件发送给朋友，朋友导入到自己的技能目录即可使用。

## 版本

- v1.4.1 (2026-11-26): 日出改用 **`Loading`(47) 单段单效果**（`pp` 整数步进 ⇒ 一颗 IC 一颗 IC 走，
  65.5 秒灌满），替掉 12 幕播放列表（每幕跳 5.8 颗、12 个预设）；`verify_shape` 增加**峰值漂移**
  判据（软渐变下边界阈值会钝化）；`wled.py` 读 `presets.json` 改容错解码（非 UTF-8 会导致
  "存了但读不到"）；**已知限制**：490 颗那条同配方下移动带仅 3%–24%，跨灯数需按屏宽百分比换算
- v1.4.0 (2026-11-26): 新增 `scripts/verify_shape.py`（**形状层验收**：亮界是否单向推进 /
  是否整条同亮同灭 / 有无静态亮块），验收升级为**四层**（通信→像素→运动→形状）；
  `apply_recipe` 支持 `start_pct`/`stop_pct` 按百分比定位段；`recipes/日出东方.json` 改为
  **12 幕亮界推进**（60 秒，一份配方同时跑 70/490 颗），实测右边界增长 58%–91%；
  文档纠正：`Sunrise` 亮区恒在中点、`Wipe` 是来回擦、`Meteor` 只亮 5% —— 都不能做单向日出
- v1.3.2 (2026-11-26): 新增 `scripts/verify_motion.py`（**变化量质心**判据：区分"变亮"与"在动"）；
  `recipes/日出东方.json` 改为**双层**（Sunrise 天亮 + Meteor 走太阳），平均亮度 8→145；
  修 Meteor 的 `sx`/`ix` 被误钳（`sx` 是速度、`ix` 是拖尾长度），并加入 `W_PINNED_EFFECTS`
- v1.3.1 (2026-11-26): 新增示例配方 `recipes/日出东方.json`（原生 Sunrise，一次 60 秒）；
  **修五色灯带日出满亮**：`color_from_palette` 把白通道钉在 `col[0].W`，恒定白底要先置 0
  （`apply_recipe.W_PINNED_EFFECTS`）；`apply_recipe` 现在按 `info.leds.lc` 自动给任何配方
  拆白 + 补 `cct`，同一份配方可直接跑 RGB / RGBW / RGBCCT；段省略 `stop` = 铺满整条；
  `Sunrise` 的 `sx` 是日出分钟数，已加入限幅豁免（原先会被钳成 50 分钟）
- v1.3.0 (2026-11-26): **统一入口 `wled.py`**（动词 + 设备别名 + 自描述 `effects`/`palettes` + `save`/`preset`/`recall` 一键复用），融合社区同类技能的交互优势；新增 `references/intent-map.md`（意图→效果，21 个效果全部在设备上核对存在，剔除本机没有的 `Forest`/`Sunset`）与 `references/done-checklist.md`（7 条交付前必查）；修 **中文预设名被存成 `\uXXXX`** （6 个脚本的 `json.dumps` 缺 `ensure_ascii=False`）与**预设落盘延迟**（改为回读确认）
- v1.2.0 (2026-11-26): **Token 架构重构** —— SKILL.md 从 978 行 / ≈11.9k token 压到
  147 行 / ≈1.9k（-84%），改为「一张路由表 + 按需加载」：静态场景、动态模版、硬规则、
  能力位、取色链路、配方格式各自独立成 `references/*.md`；补「典型任务真实 token 成本」表，
  并明确**效果清单以设备 `/json/effects` 为准、不要整目录读官方镜像（≈66k）**；
  新增「存到手机 / 下次一键触发」的零 token 路径（设备端预设 + WLED App / 米家场景）
- v1.1.0 (2026-11-26): **更名 WLED Skills**（原 `doubao-wled-skill`，仓库改名保留历史与旧链接跳转）；新增 `references/device-safety.md` 写保护层（写前 `--backup`、危险 GET、非幂等不重试、禁止残缺 `/settings` 表单）；场景名之外支持**中英文颜色名**兜底（红/暖白/blue/cyan…）；README 中英双版
- v1.0.8 (2026-11-25): **全类型 WLED 自适应** —— 按固件上报的 `info.leds.lc` 能力位
  （`0x01`=RGB / `0x02`=W / `0x04`=CCT）决定出码形状，主推 WS2805 五色自动拆白并带 `cct`，
  WS2815/PWM 等纯 RGB 灯带走 3 通道；`apply_recipe.py` 新增 `--ip` 指定目标灯带、
  并改为**以设备实际灯数为准**（原先 config 的 70 压过设备，490 颗的配方会被判非法）；
  `scene_palette.py` 新增 `--probe` / `--ip` / `--off`，灯数与设备不符时拒绝下发
- v1.0.7 (2026-11-25): **技能可被 WorkBuddy / 豆包直接读取**（注册为 `~/.workbuddy/skills/wled-light-recipe`，
  软链回仓库保持单一真源）；新增 Rule 11「测完必须熄灯 + `--release` 归还 UDP 同步接收」；
  文档补两条灯的现场口径（70 颗 / 490 颗不要混用）；`config.json` 停止入库（含内网 IP，改为 gitignore）
- v1.0.6 (2026-11-25): 新增**场景配色快速通道** —— 14 组氛围配色内置离线表，
  说一句场景名直接出三色渐变配方（`scripts/scene_palette.py`），走既有下发门禁；
  同时放开 Shimmer 的 `sx=0` 静态档（`_EFFECT_AXIS_EXEMPT` 原先把 0 钳到 40，
  会把"静止渐变"悄悄变成慢动作）
- v1.0.5 (2026-09-16): 视频取色光配方、按名称复用、办公模式写入门禁、局域网主从同步、
  实时流优先级接管；并修复陈旧缓存指向离线设备、实时模式下预设保存被丢弃、
  同步广播缺少运行时开关、自定义调色板 ID 映射错误等静默失效问题。
  另新增**已验证效果模版库**（`references/effect-templates.md`）与**丝滑度客观判据**
  （`verify_live.py` 的 `smooth_motion` / 静止帧占比），并固化「换场景不重置动画」
  这一硬约束（动画段 `ix`/`c1` 必须恒定）
- v1.0.4 (2026-08): 效果 ID 按设备实际效果名解析，禁止硬编码
- v1.0.0 (2026-07-29): 初始版本，支持图片搜索、颜色解析、调色板上传、预设保存、光配方导入导出
