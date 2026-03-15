# SIC API Reference

All devices are accessed through typed vtable pointers returned by index accessors.
Index 0 = first device of that type registered for the current board.

## Core

```c
#include <sic/sic.h>

/* Initialise SIC for a specific board. Returns 1 on success, 0 if no board. */
int sic_begin(const sic_board_t* board, const sic_begin_opts_t* opts);
```

Boards are declared in `src/boards/` and selected via build flags (e.g. `-DSIC_TARGET_TPAGER=1`).

---

## Keyboard

```c
#include <sic/input/kscan.h>

const kscan_t* sic_kbd(int index);

/* kscan vtable */
kbd->v->scan(kbd);                          /* update internal bitmap */
uint64_t bm = kbd->v->bitmap(kbd, row);     /* bitmask of pressed keys in row */
```

---

## Rotary Encoder

```c
#include <sic/input/encoder.h>

const encoder_t* sic_encoder(int index);

/* encoder vtable */
int delta = enc->v->read(enc);    /* consume accumulated steps since last call */
int btn   = enc->v->button(enc);  /* 1 = pressed, 0 = released */
```

---

## Microphone

```c
#include <sic/audio/mic.h>

const mic_t* sic_mic(int index);

/* mic vtable */
int rc = mic->v->start(mic, 16000);               /* open I2S + init codec; rc<0 = error */
int n  = mic->v->read(mic, int16_t* buf, int n);  /* returns frames read, <0 = error */
```

---

## Speaker Amp

```c
#include <sic/audio/amp.h>

const amp_t* sic_amp(int index);

/* amp vtable */
amp->v->enable(amp, 1);   /* unmute DAC and power speaker amp */
amp->v->enable(amp, 0);   /* mute */
```

Audio data is sent to the codec via the HAL directly:

```c
#include <sic/audio.h>

int sic_codec_write(const int16_t* mono, int n);  /* mono frames → stereo I2S TX */
```

---

## Battery

```c
#include <sic/sic.h>

sic_battery_t bat = {0};
int rc = sic_battery_read(&bat);
/* bat.voltage_v — float, e.g. 3.85 */
/* bat.percent   — 0..100, or -1 if unavailable */
```

---

## I2C Bus

```c
#include <sic/bus/i2c_bus.h>

int sic_i2c_write    (int bus, uint8_t addr, const uint8_t* data, int len);
int sic_i2c_read     (int bus, uint8_t addr, uint8_t* data, int len);
int sic_i2c_writeread(int bus, uint8_t addr,
                      const uint8_t* wr, int wlen,
                      uint8_t* rd, int rlen);
```

Returns bytes transferred on success, negative on error.

---

## GPIO Bus

```c
#include <sic/bus/gpio_bus.h>

void sic_gpio_mode       (int pin, int output);   /* 1=output, 0=input */
void sic_gpio_mode_pullup(int pin);
void sic_gpio_write      (int pin, int val);
int  sic_gpio_read       (int pin);
```

---

## Delay

```c
void sic_delay_ms(int ms);
```
