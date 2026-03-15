# Design Invariants

These rules must hold in every commit. They define what SIC is and what it will never become.

## API

- **Single include**: all public API reachable from `#include <sic/sic.h>`.
- **Typed vtable accessors**: devices returned as `const foo_t*` with a `v` vtable pointer.
  Caller code: `sic_mic(0)->v->start(mic, 16000)`. Never raw function pointers in application code.
- **Error codes**: 0 or positive = success; negative = error. Never platform error enums.
- **Index-based multi-device**: `sic_mic(0)`, `sic_mic(1)`, … — no global singletons in the API layer.

## Platform isolation

- **Drivers** (`src/drivers/`) and **core** (`src/core/`): pure C99. No Arduino, FreeRTOS, or ESP-IDF headers.
  Only `stdlib.h`, `string.h`, `stdint.h`, `stdbool.h`, and SIC HAL (`sic_i2c_*`, `sic_gpio_*`, `sic_delay_ms`).
- **Backends** (`src/backends/arduino/`, `src/bus/*_arduino.*`): platform code lives here only,
  compiled exclusively when `defined(ARDUINO) || defined(SIC_BACKEND_ARDUINO)`.
- The goal: `src/drivers/` compiles on a PC with `gcc -std=c99`. No exceptions.

## Initialisation

- **Probe never does I/O**: `probe()` must always succeed if the config struct is valid.
  Hardware communication is deferred to the first vtable call (`start()`, `enable()`, etc.).
  This prevents boot hangs when a device is absent or not yet powered.
- **MCLK before I2C**: for I2S codecs, the I2S peripheral (and MCLK) must be started and
  allowed to stabilise (≥15ms) before writing I2C registers. Probe defers this to `mic.start()`.

## Memory

- No exceptions, no RTTI.
- Small heap usage. Drivers allocate one singleton context at first probe; no per-call allocation.
- Driver registry limited to 64 entries (compile-time array) — sufficient for any real board.

## Scope

- SIC is not an RTOS. It does not manage threads, tasks, or scheduling.
- SIC is not a full BSP. It abstracts specific ICs and buses; it does not own the main loop.
