# SIC API Reference (snapshot)

## Core
- `int sic_begin(const void* board_desc, const sic_begin_opts_t* opts);`
- `sic_ctx_t* sic_ctx();` returns singleton context.
- Logging: `sic_log_info`, `sic_log_warn`, `sic_log_err`.

## Audio
- Amp: `int sic_amp_enable(int on);`
- Mic: `int sic_mic_start(int sample_hz, int frames_per_chunk);`
  `int sic_mic_read(int16_t* out_frames, size_t max_frames, int timeout_ms);`
  `int sic_mic_stop();`

## Input (keyboard scan)
- `sic_kscan_handle_t* sic_kscan_open(...);`
- `int sic_kscan_read_bitmap(uint64_t* rows, size_t nrows);`

## Bus
- GPIO bus + I2C shims (`sic/bus/*`).

## Power
- `int sic_battery_read(sic_battery_t* out);`
- TP4057 basic charger state.
