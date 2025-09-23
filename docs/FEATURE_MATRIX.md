# Feature Matrix

| Area | Status | Files |
| --- | --- | --- |
| System info | ✅ | `lib/SIC/src/hal/hal_core.cpp` |
| I2C bus | ✅ (Arduino backend) | `lib/SIC/src/bus/i2c_bus_arduino.cpp` |
| GPIO bus/delay | ✅ | `lib/SIC/src/bus/gpio_bus_arduino.c`, `lib/SIC/src/bus/delay_arduino.c` |
| Keyboard 74HC138 | ✅ (Cardputer) | `lib/SIC/src/drivers/input/kbd_74hc138_gpio.c`, `.../cardputer_keymap.c` |
| Audio MIC (PDM/I2S) | ✅ (Arduino backend) | `lib/SIC/src/backends/arduino/sic_arduino_audio.cpp`, `lib/SIC/src/audio/*` |
| Audio AMP (GPIO) | ✅ | `lib/SIC/include/sic/audio/amp.h`, used in `hal_core.cpp` |
| Battery ADC | ✅ (simple) | `lib/SIC/src/power/battery_adc.cpp` |
| Charger TP4057 | ⚠️ minimal | `lib/SIC/src/drivers/power/tp4057.c` |
| IR TX | ⚠️ dummy only | `lib/SIC/src/drivers/dummy.c` |
| SD storage | ❌ not implemented | headers only in `lib/SIC/include/sic/storage/` |
| Autoregistration | 🚫 omitted in RC | — |
