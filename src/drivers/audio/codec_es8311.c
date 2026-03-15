/*
 * codec_es8311.c — ES8311 audio codec driver for SIC.
 *
 * Platform-agnostic: uses only SIC HAL functions (sic_i2c_write,
 * sic_codec_open/read/write/close, sic_delay_ms).
 *
 * Critical init order: I2S (MCLK) must be running before I2C register
 * writes, otherwise the ES8311 may not respond.  es_mic_start() opens
 * the I2S peripheral first, waits for MCLK to stabilise, then programs
 * the codec over I2C.
 *
 * Register sequence derived from LilyGoLib / ESP-ADF for 256×Fs MCLK,
 * slave mode (ESP32-S3 is I2S master, ES8311 is slave).
 */

#include <stdlib.h>
#include <string.h>
#include "sic/sic.h"
#include "sic/audio/mic.h"
#include "sic/audio/amp.h"
#include "sic/audio/codec_es8311.h"
#include "sic/bus/i2c_bus.h"
#include "sic/bus/delay.h"

/* ── ES8311 I2C register init ─────────────────────────────────────────── */

/*
 * Sequence matches LilyGoLib's esp_codec / ESP-ADF es8311_open() for
 * slave mode, analog mic, 256×Fs external MCLK (MCLK = 4.096 MHz at 16 kHz).
 *
 * Notable differences from naive ESP-ADF copies:
 *  - REG44=0x08 first (GPIO1 as DMIC_CLK/nSPKEN — must be set before powerup)
 *  - REG00=0x80 is a soft power-up, NOT a hard reset (0x1F would make the
 *    chip reboot and temporarily stop ack-ing I2C)
 *  - REG01 written twice: 0x30 (MCLK on, pre-config), then 0x3F (slave mode)
 *  - Analog mic path: REG14=0x1A clears the digital-mic bit
 */
static const unsigned char k_init[][2] = {
    /* ── GPIO / port config (must precede power-up) ──────────────── */
    {0x44, 0x08},  /* GPIO1 = nSPKEN function, initial config         */
    /* ── Clock manager (pre-power-up) ───────────────────────────── */
    {0x01, 0x30},  /* SYSCLK: ext MCLK in, MCLK enabled, pre-div /1  */
    {0x02, 0x00},  /* MCLK pre-divider /1                             */
    {0x03, 0x10},  /* BCLK: output mode                               */
    /* ── ADC gain (set before power-up) ─────────────────────────── */
    {0x16, 0x24},  /* MIC PGA gain 18 dB (initial)                    */
    /* ── LRCK divider for 256×Fs ────────────────────────────────── */
    {0x04, 0x01},  /* LRCK H = 0x01  → total div = 0x100 = 256       */
    {0x05, 0x00},  /* LRCK L = 0x00                                   */
    /* ── System registers ───────────────────────────────────────── */
    {0x0B, 0x00},
    {0x0C, 0x00},
    {0x10, 0x1F},  /* analog: inputs disabled (safe state at boot)    */
    {0x11, 0x7F},
    /* ── Soft power-up (not hard reset) ─────────────────────────── */
    {0x00, 0x80},  /* RESET_REG: soft power-up sequence               */
    /* ── Slave mode + MCLK from external pin ────────────────────── */
    {0x01, 0x3F},  /* SYSCLK: ext MCLK, slow-clk off, slave mode      */
    /* ── More analog init ───────────────────────────────────────── */
    {0x13, 0x10},
    {0x1B, 0x0A},
    {0x1C, 0x6A},
    {0x44, 0x08},  /* GPIO1 = nSPKEN, no DAC2ADC loopback              */
    /* ── ADC path (analog mic) ──────────────────────────────────── */
    {0x17, 0xBF},  /* ADC digital volume = 0 dB                       */
    {0x0E, 0x02},  /* analog power: MIC bias on, reference on         */
    {0x12, 0x00},  /* DAC analog power on                             */
    {0x14, 0x1A},  /* ADC ctrl: analog mic path (clears digital-mic bit) */
    {0x0D, 0x01},  /* VMID normal, IBIAS off                          */
    {0x15, 0x40},
    /* ── DAC path ───────────────────────────────────────────────── */
    {0x31, 0x00},  /* DAC_CTRL1: unmute (bits 6+5 = 0)                */
    {0x32, 0xBF},  /* DAC digital volume = 0 dB                       */
    {0x37, 0x08},  /* DAC_CTRL7: EQ bypass, ramp off                  */
    {0x45, 0x00},  /* GP: normal operating mode                       */
};

#define ES8311_REG_DAC_CTRL 0x31u
#define ES8311_DACMUTE_BITS 0x60u  /* bits 6+5: digital + analog mute */

/* ── Shared singleton ──────────────────────────────────────────────────── */

typedef struct {
    int           i2c_bus;
    unsigned char i2c_addr;
    int           pin_mclk, pin_bclk, pin_ws, pin_dout, pin_din;
    int           sample_rate;
    int           initialized;   /* 0=pending  1=ok  -1=failed (retryable) */
    int           i2s_open;
} es8311_ctx_t;

static es8311_ctx_t* g_es = NULL;

/* Returns 0 on success, -1 on error (normalises sic_i2c_write's bytes-written return). */
static int es_i2c_write(es8311_ctx_t* e, unsigned char reg, unsigned char val) {
    unsigned char buf[2] = { reg, val };
    return sic_i2c_write(e->i2c_bus, e->i2c_addr, buf, 2) < 0 ? -1 : 0;
}

/*
 * Attempt I2C register init.  Resets initialized=0 so the caller can
 * retry after ensuring MCLK is running.  Does NOT permanently latch -1.
 */
static int es_chip_init(es8311_ctx_t* e) {
    if (e->initialized == 1) return 0;
    e->initialized = 0;  /* allow retry each time */

    for (unsigned i = 0; i < sizeof(k_init)/sizeof(k_init[0]); i++) {
        if (es_i2c_write(e, k_init[i][0], k_init[i][1]) != 0) {
            return -1;  /* caller retries on next invocation */
        }
        sic_delay_ms(2);
    }
    e->initialized = 1;
    return 0;
}

static es8311_ctx_t* get_or_create(const sic_es8311_cfg_t* cfg) {
    if (g_es) return g_es;
    g_es = (es8311_ctx_t*)calloc(1, sizeof *g_es);
    if (!g_es) return NULL;
    g_es->i2c_bus     = cfg->i2c_bus;
    g_es->i2c_addr    = cfg->i2c_addr;
    g_es->pin_mclk    = cfg->pin_mclk;
    g_es->pin_bclk    = cfg->pin_bclk;
    g_es->pin_ws      = cfg->pin_ws;
    g_es->pin_dout    = cfg->pin_dout;
    g_es->pin_din     = cfg->pin_din;
    g_es->sample_rate = 16000;
    return g_es;
}

/* ── mic_t vtable ──────────────────────────────────────────────────────── */

/*
 * Open I2S first (starts MCLK on pin_mclk), wait for the ES8311 oscillator
 * to lock, then write I2C registers.  Some ES8311 revisions do not respond
 * to I2C reliably until MCLK has been stable for ~10 ms.
 */
static int es_mic_start(const void* self, int hz) {
    es8311_ctx_t* e = (es8311_ctx_t*)((const mic_t*)self)->impl;
    if (!e) return -1;

    /* 1. Start I2S / MCLK first */
    if (!e->i2s_open || e->sample_rate != hz) {
        if (e->i2s_open) { sic_codec_close(); e->i2s_open = 0; }
        e->sample_rate = hz;
        int r = sic_codec_open(e->pin_mclk, e->pin_bclk, e->pin_ws,
                               e->pin_dout, e->pin_din, hz);
        if (r != 0) return -1;
        e->i2s_open = 1;
        sic_delay_ms(15);  /* let MCLK stabilise before I2C init */
    }

    /* 2. Init codec over I2C (retried each call until it succeeds) */
    return es_chip_init(e);
}

static int es_mic_read(const void* self, short* buf, int n) {
    es8311_ctx_t* e = (es8311_ctx_t*)((const mic_t*)self)->impl;
    if (!e || !e->i2s_open || e->initialized != 1) return 0;
    return sic_codec_read(buf, n, 200);
}

static const struct mic_vtbl_s MIC_VT = { es_mic_start, es_mic_read };

/* ── amp_t vtable ──────────────────────────────────────────────────────── */

/*
 * amp.enable() also ensures MCLK + I2C init before muting/unmuting.
 * The physical speaker amp (gated by XL9555 EXPANDS_AMP_EN) is kept
 * always-on via board_tpager preinit; this call only controls the
 * ES8311 DAC mute register.
 */
static void es_amp_enable(const void* self, int on) {
    es8311_ctx_t* e = (es8311_ctx_t*)((const amp_t*)self)->impl;
    if (!e) return;
    /* Ensure I2S/MCLK and codec init if not already done */
    if (!e->i2s_open) {
        int r = sic_codec_open(e->pin_mclk, e->pin_bclk, e->pin_ws,
                               e->pin_dout, e->pin_din, e->sample_rate);
        if (r != 0) return;
        e->i2s_open = 1;
        sic_delay_ms(15);
    }
    if (es_chip_init(e) != 0) return;
    unsigned char v = on ? 0x00u : ES8311_DACMUTE_BITS;
    es_i2c_write(e, ES8311_REG_DAC_CTRL, v);
}

static const struct amp_vtbl_s AMP_VT = { es_amp_enable };

/* ── Probe — always succeeds (no I2C / I2S at probe time) ──────────────── */

static mic_t g_mic_inst;
static amp_t g_amp_inst;

static int probe_mic(const void* icdesc, void** out) {
    const sic_board_ic_t* d = (const sic_board_ic_t*)icdesc;
    if (!d || !d->hint || strcmp(d->hint, "codec_es8311") != 0) return -1;
    es8311_ctx_t* ctx = get_or_create((const sic_es8311_cfg_t*)d->cfg);
    if (!ctx) return -1;
    g_mic_inst.v    = &MIC_VT;
    g_mic_inst.impl = ctx;
    *out = &g_mic_inst;
    return 0;
}

static int probe_amp(const void* icdesc, void** out) {
    const sic_board_ic_t* d = (const sic_board_ic_t*)icdesc;
    if (!d || !d->hint || strcmp(d->hint, "codec_es8311") != 0) return -1;
    es8311_ctx_t* ctx = get_or_create((const sic_es8311_cfg_t*)d->cfg);
    if (!ctx) return -1;
    g_amp_inst.v    = &AMP_VT;
    g_amp_inst.impl = ctx;
    *out = &g_amp_inst;
    return 0;
}

/* ── Driver registration ───────────────────────────────────────────────── */

static const sic_driver_t DRV_MIC = {
    "codec_es8311", SIC_F_MIC, probe_mic, NULL
};
static const sic_driver_t DRV_AMP = {
    "codec_es8311", SIC_F_AMP, probe_amp, NULL
};

void sic_register_driver_codec_es8311(void) {
    sic_registry_register(&DRV_MIC);
    sic_registry_register(&DRV_AMP);
}
