# Feature Matrix

| Area | Status | Driver / Files |
|------|--------|----------------|
| I2C bus | ✅ | `src/bus/i2c_bus_arduino.cpp` |
| GPIO bus / delay | ✅ | `src/bus/gpio_bus_arduino.c`, `src/bus/delay_arduino.c` |
| Keyboard — TCA8418 (I2C matrix) | ✅ | `src/drivers/input/kbd_tca8418_i2c.c` |
| Keyboard — 74HC138 (GPIO matrix) | ✅ | `src/drivers/input/kbd_74hc138_gpio.c` |
| Rotary encoder — GPIO | ✅ | `src/drivers/input/encoder_gpio.c` |
| Audio codec — ES8311 (I2S mic + amp) | ✅ | `src/drivers/audio/codec_es8311.c` |
| Audio I2S backend | ✅ | `src/backends/arduino/sic_arduino_audio.cpp` |
| Battery — BQ27220 (I2C fuel gauge) | ✅ | `src/drivers/power/bq27220.c` |
| Battery — ADC (simple voltage divider) | ✅ | `src/power/battery_adc.cpp` |
| Charger — TP4057 | ⚠️ minimal | `src/drivers/power/tp4057.c` |
| IR TX | ⚠️ dummy only | `src/drivers/dummy.c` |
| SD storage | ❌ not implemented | headers only in `include/sic/storage/` |
| Driver autoregistration | ✅ | `src/core/autoreg.c` |
| Driver registry + typed accessors | ✅ | `src/core/registry.c` |
