# Troubleshooting

**No serial logs** — match baud rate, avoid long blocking calls in `setup()`.

**Duplicate symbols** — add `-DSIC_DISABLE_DUMMY=1` to disable the dummy fallback drivers.

**Keyboard dead** — check wiring and pull-ups; adjust debounce and scan period in `board_*.c`.

**`hw` shows `mic: none` or `amp: none`** — codec probe never fails; if these show `none` the
driver is not registered. Check that `-DSIC_DRV_CODEC_ES8311=1` is in `build_flags` and the
board descriptor includes a `SIC_F_MIC` and `SIC_F_AMP` entry pointing to `"codec_es8311"`.

**`mic: start failed`** — I2S open or ES8311 I2C init failed. Verify:
- MCLK/BCLK/WS/DOUT/DIN pin numbers match hardware.
- ES8311 is at I2C address `0x18` (run `i2c` command to scan).
- `use_apll = false` in `sic_arduino_audio.cpp` (APLL can fail on ESP32-S3).
- I2S is opened *before* I2C register writes — MCLK must be running for ES8311 to respond.

**Mic silent / stuck at max value** — check `REG44` is `0x08` (not `0x58`). Value `0x58`
enables the DAC2ADC internal loopback, routing playback back into the ADC input.

**Amp produces no sound** — the DAC mute register is `REG31` (bits 6+5), not `REG37`.
Ensure `es_amp_enable` writes `0x00` to `REG31` to unmute, and that audio data is being
written to the I2S TX line via `sic_codec_write()` while the amp is enabled.

**Encoder not detected** — add `-DSIC_DRV_ENCODER_GPIO=1` and verify `sic_encoder_cfg_t`
pin numbers in `board_*.c`. GPIO pull-up is applied automatically by `encoder_gpio.c`.
