/*
 * kbd_tca8418_i2c.c — SIC kscan driver for the TCA8418 I2C keyboard controller.
 *
 * The TCA8418 handles full matrix scanning internally and exposes key events
 * via a 10-deep FIFO over I2C.  The host only needs I2C + an optional INT pin.
 *
 * Build flags consumed:
 *   TCA8418_I2C_BUS   — SIC I2C bus index (default 0)
 *   TCA8418_I2C_ADDR  — 7-bit address (default 0x34)
 *   TCA8418_INT_PIN   — active-low interrupt GPIO (default SIC_NOPIN = polled)
 *   TCA8418_ROWS      — number of rows in use (default 8)
 *   TCA8418_COLS      — number of cols in use (default 10)
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "sic/input/kscan.h"
#include "sic/bus/i2c_bus.h"
#include "sic/bus/gpio_bus.h"
#include "sic/sic_board.h"
#include "sic/sic_registry.h"
#include "sic/sic.h"

/* ---- Build-flag defaults ---- */
#ifndef TCA8418_I2C_BUS
#  define TCA8418_I2C_BUS  0
#endif
#ifndef TCA8418_I2C_ADDR
#  define TCA8418_I2C_ADDR 0x34
#endif
#ifndef TCA8418_INT_PIN
#  define TCA8418_INT_PIN  SIC_NOPIN
#endif
#ifndef TCA8418_ROWS
#  define TCA8418_ROWS 8
#endif
#ifndef TCA8418_COLS
#  define TCA8418_COLS 10
#endif

/* ---- TCA8418 register map ---- */
#define TCA8418_REG_CFG        0x01  /* Configuration */
#define TCA8418_REG_INT_STAT   0x02  /* Interrupt status (write 1 to clear) */
#define TCA8418_REG_KEY_LCK_EC 0x03  /* bits[3:0] = FIFO event count */
#define TCA8418_REG_KEY_EVT_A  0x04  /* First FIFO entry (10 slots: 0x04-0x0D) */
#define TCA8418_REG_KP_GPIO1   0x1D  /* Row  0-7  keypad-vs-GPIO selection (1=keypad) */
#define TCA8418_REG_KP_GPIO2   0x1E  /* Col  0-7  keypad-vs-GPIO selection (1=keypad) */
#define TCA8418_REG_KP_GPIO3   0x1F  /* Col  8-9  keypad-vs-GPIO selection (bits[1:0]) */

/* CFG bits */
#define TCA8418_CFG_KE_IEN   (1u << 0)  /* Key-event interrupt enable */
#define TCA8418_CFG_INT_CFG  (1u << 4)  /* INT pin level: 0=active-low */

/* KEY_EVENT encoding */
#define TCA8418_EVT_PRESS    (1u << 7)
#define TCA8418_EVT_CODE(b)  ((b) & 0x7F)  /* key code 1-80 (or 97-114 for GPI) */

/* Max key code we map to the 64-bit bitmap */
#define TCA8418_MAX_CODE 64

/* ---- I2C helpers ---- */
static int reg_read(uint8_t reg, uint8_t* val) {
    return sic_i2c_writeread(TCA8418_I2C_BUS,
                             TCA8418_I2C_ADDR,
                             &reg, 1,
                             val, 1);
}
static int reg_write(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg, val };
    return sic_i2c_write(TCA8418_I2C_BUS, TCA8418_I2C_ADDR, buf, 2);
}

/* ---- Driver state ---- */
typedef struct {
    unsigned long long state_bm;    /* live key state */
    unsigned long long pending_bm;  /* press events accumulated since last read_bitmap call */
} kbd_tca8418_t;

/* ---- kscan_read_bitmap implementation ---- */
static int tca8418_read_bitmap(const struct kscan_s* self, unsigned long long* out) {
    kbd_tca8418_t* st = (kbd_tca8418_t*)self->impl;

    /* Check whether an event is pending (skip if INT pin is high) */
    if (TCA8418_INT_PIN != SIC_NOPIN && sic_gpio_read(TCA8418_INT_PIN) != 0) {
        *out = st->state_bm | st->pending_bm;
        st->pending_bm = 0;
        return 0;
    }

    /* Drain the event FIFO */
    uint8_t ec_reg = 0;
    if (reg_read(TCA8418_REG_KEY_LCK_EC, &ec_reg) < 0) return -1;
    int count = (int)(ec_reg & 0x0F);

    for (int i = 0; i < count; ++i) {
        uint8_t evt = 0;
        if (reg_read(TCA8418_REG_KEY_EVT_A, &evt) < 0) break;
        int code = (int)TCA8418_EVT_CODE(evt);
        if (code < 1 || code > TCA8418_MAX_CODE) continue;
        int bit = code - 1;
        if (evt & TCA8418_EVT_PRESS) {
            st->state_bm   |=  (1ULL << bit);
            st->pending_bm |=  (1ULL << bit); /* latch press — survives same-poll release */
        } else {
            st->state_bm   &= ~(1ULL << bit); /* release clears live state only */
        }
    }

    /* Clear K_INT in INT_STAT (write-1-to-clear) */
    reg_write(TCA8418_REG_INT_STAT, 0x01);

    *out = st->state_bm | st->pending_bm;
    st->pending_bm = 0;
    return 0;
}

/* ---- vtable ---- */
static int read_key_stub(const void* self) { (void)self; return -1; }
static const struct kscan_vtbl_s TCA8418_VT = {
    .read_key    = read_key_stub,
    .read_bitmap = tca8418_read_bitmap,
};

/* ---- Probe / make ---- */
static const struct kscan_s* make(const sic_kscan_cfg_t* cfg) {
    /* Configure rows 0-7 and cols 0-9 as keypad */
    if (reg_write(TCA8418_REG_KP_GPIO1, 0xFF) < 0) return NULL;  /* rows 0-7 */
    if (reg_write(TCA8418_REG_KP_GPIO2, 0xFF) < 0) return NULL;  /* cols 0-7 */
    if (reg_write(TCA8418_REG_KP_GPIO3, 0x03) < 0) return NULL;  /* cols 8-9 */

    /* Enable key-event interrupt; INT pin active-low */
    if (reg_write(TCA8418_REG_CFG, TCA8418_CFG_KE_IEN) < 0) return NULL;

    if (TCA8418_INT_PIN != SIC_NOPIN) {
        sic_gpio_mode_pullup(TCA8418_INT_PIN); /* input + pull-up (INT is open-drain) */
    }

    kbd_tca8418_t* st = (kbd_tca8418_t*)calloc(1, sizeof *st);
    if (!st) return NULL;

    struct kscan_s* k = (struct kscan_s*)calloc(1, sizeof *k);
    if (!k) { free(st); return NULL; }

    k->v             = &TCA8418_VT;
    k->impl          = st;
    k->keymap        = cfg ? cfg->keymap        : NULL;
    k->keymap_alt    = cfg ? cfg->keymap_alt    : NULL;
    k->modifier_mask = cfg ? cfg->modifier_mask : 0;
    k->shift_mask    = cfg ? cfg->shift_mask    : 0;
    return k;
}

static int probe(const void* icdesc, void** out) {
    const sic_board_ic_t* d = (const sic_board_ic_t*)icdesc;
    if (!d || !d->hint) return -1;
    if (strcmp(d->hint, "kbd_tca8418") != 0) return -1;
    *out = (void*)make((const sic_kscan_cfg_t*)d->cfg);
    return (*out) ? 0 : -1;
}

static const sic_driver_t DRV = {
    .name     = "kbd_tca8418",
    .function = SIC_F_KSCAN,
    .probe    = probe,
    .remove   = NULL
};

#ifdef __cplusplus
extern "C" {
#endif
void sic_register_driver_kbd_tca8418(void) { sic_registry_register(&DRV); }
#ifdef __cplusplus
}
#endif

/* kscan_read_bitmap dispatches through self->v->read_bitmap (kscan_dispatch.c).
 * No global symbol defined here — both this driver and kbd_74hc138 can coexist. */
