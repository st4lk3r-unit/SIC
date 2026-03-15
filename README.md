
# SIC — Simple Interface Components (v1.0.0)

[![Build](https://img.shields.io/badge/build-GitHub%20Actions-inactive)](#)
[![License](https://img.shields.io/badge/license-WTFPL-blue)](#)

SIC is a **single-include** hardware abstraction layer for ESP32-class boards.
Consume via `#include <sic/sic.h>`.

## ✨ 5‑Minute Quickstart

**Option A — Run the example (Cardputer):**
```bash
cd SIC/examples/Cardputer-Demo
pio run -e cardputer-io-stable -t upload
pio device monitor -b 115200
```

**Option B — Use SIC in your existing PlatformIO project:**
```ini
; platformio.ini
lib_deps =
  ; if using as submodule, add a local path:
  ; ../SIC
build_flags =
  -DSIC_TARGET_CARDPUTER=1
  -DI2C_SDA_PIN=41 -DI2C_SCL_PIN=42
  -DMIC_DATA=46  -DMIC_CLK=43
  -DIR_TX_PIN=44
```
```cpp
// src/main.cpp
#include <Arduino.h>
#include <sic/sic.h>

void setup(){
  Serial.begin(115200);
  int rc = sic_begin(nullptr, nullptr);
  Serial.printf("[OK] sic_begin rc=%d\n", rc);
}
void loop(){}
```

**Option C — Unit tests (native, minimal):**
```bash
cd SIC/tests/native
make
./out/test_sic_native
```

## Repo Layout
- `include/` — public headers (`#include <sic/sic.h>`)
- `src/`     — boards, drivers, HAL
- `examples/` — ready-to-build demos (PlatformIO)
- `docs/`    — feature matrix, porting guide, pins, API, troubleshooting
- `templates/` — driver authoring template (copy‑paste to start new drivers)
- `tests/`   — native tests (very small sanity checks)
- `.github/workflows/` — CI build for examples

## Status
See `docs/FEATURE_MATRIX.md` for what’s implemented vs. stubbed.
See `docs/DESIGN_INVARIANTS.md` for guarantees and non-goals.

## Contributing
Read `CONTRIBUTING.md` and open a PR! Also see `docs/COOKBOOK_BOARD_BRINGUP.md`.
