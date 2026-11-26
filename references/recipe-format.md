# 配方文件格式与展示约定

> 由 SKILL.md 拆分而来，按需读取；不要整篇灌进上下文。

## Recipe File Format

All recipe files use a portable JSON format (`wled-skills-recipe-v1`):

```json
{
  "name": "Jasmine Blooms",
  "description": "Jasmine flower colors",
  "version": "2.0",
  "format": "wled-skills-recipe-v1",
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
