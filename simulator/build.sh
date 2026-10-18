#!/usr/bin/env bash
# 构建 WLED FX 仿真核心（Linux host，无 ESP32 / 无 Qt / 无 platformio）
set -euo pipefail
cd "$(dirname "$0")"
OUT=build; mkdir -p $OUT
INC="-I. -Isrc -Isrc/dependencies/fastled_slim"

echo "==> fastled_slim (CRGB / CRGBPalette16)"
g++ -std=c++17 -O2 -c src/dependencies/fastled_slim/fastled_slim.cpp $INC -o $OUT/fastled_slim.o

echo "==> FX_fcn.cpp  (221 个效果的数学核心)"
g++ -std=c++17 -O2 -c src/FX_fcn.cpp $INC -o $OUT/FX_fcn.o

if [[ "${1:-}" == "--all" ]]; then
  echo "==> 其余单元（需要更多 ESP shim，见 README）"
  for u in wled_math palettes colors FX; do
    g++ -std=c++17 -O2 -c src/$u.cpp $INC -o $OUT/$u.o || echo "  [skip] $u"
  done
fi
echo "==> 完成"; ls -la $OUT
