/*
 * sic_core.c — Platform-agnostic SIC lifecycle and capability layer.
 *
 * No Arduino / ESP-IDF / platform headers here.
 * Platform-specific implementations (sic_sysinfo, GPIO, I2C, delay) live in
 * src/backends/<platform>/ and src/bus/*_<backend>.c.
 */

/* Suppress audio compat shim — not needed in the agnostic core. */
#ifndef SIC_AUDIO_NO_COMPAT
#define SIC_AUDIO_NO_COMPAT 1
#endif

#include "sic/hal.h"
#include "sic/sic_registry.h"
#include "sic/input/kscan.h"
#include "sic/audio/amp.h"
#include "sic/audio/mic.h"
#include "sic/power/switch.h"
#include "sic/power/charger.h"
#include "sic/ir/ir_tx.h"
#include "sic/storage/sd.h"
#include "sic/bus/i2c_bus.h"

/* ── Capability cache ─────────────────────────────────────────────────── */

static int g_cap_present[SIC_CAP_COUNT] = {0};
static int g_cap_counts[SIC_CAP_COUNT]  = {0};

static void refresh_caps(void) {
    g_cap_counts[SIC_CAP_PWR_SW]    = (sic_pwr_sw(0)   != NULL);
    g_cap_counts[SIC_CAP_AUDIO_AMP] = (sic_amp(0)      != NULL);
    g_cap_counts[SIC_CAP_MIC]       = (sic_mic(0)      != NULL);
    g_cap_counts[SIC_CAP_CHARGER]   = (sic_charger(0)  != NULL);
    g_cap_counts[SIC_CAP_KSCAN]     = (sic_kbd(0)      != NULL);
    g_cap_counts[SIC_CAP_IR_TX]     = (sic_ir_tx(0)    != NULL);
    g_cap_counts[SIC_CAP_SD]        = (sic_sd(0)       != NULL);
    for (int i = 0; i < SIC_CAP_COUNT; i++)
        g_cap_present[i] = g_cap_counts[i] > 0;
}

/* ── Lifecycle ────────────────────────────────────────────────────────── */

int sic_begin(const void* board_desc, const sic_begin_opts_t* opts) {
    int rc = sic_begin_legacy((const sic_board_t*)board_desc, opts);
    refresh_caps();
    return rc;
}

void sic_end(void) {}

/* ── Capability queries ───────────────────────────────────────────────── */

int sic_has(sic_cap_t cap) {
    return (cap >= 0 && cap < SIC_CAP_COUNT) ? g_cap_present[cap] : 0;
}

int sic_count_cap(sic_cap_t cap) {
    return (cap >= 0 && cap < SIC_CAP_COUNT) ? g_cap_counts[cap] : 0;
}

/* ── Peripheral helpers ───────────────────────────────────────────────── */

int sic_ir_send_nec(uint32_t code) {
    const ir_t* ir = sic_ir_tx(0);
    if (!ir || !ir->v || !ir->v->send_nec) return -1;
    ir->v->send_nec(ir, code);
    return 0;
}

int sic_sd_present(void) { return sic_sd(0) != NULL; }

int sic_i2c_begin(int sda, int scl, uint32_t hz) {
    return sic_i2c_begin_bus(0, sda, scl, hz);
}

int sic_i2c_scan(uint8_t* addrs, int max) {
    return sic_i2c_scan_bus(0, addrs, max);
}

int sic_charger_state(sic_chg_state_t* out) {
    if (!out) return -1;
    const charger_t* c = sic_charger(0);
    if (!c || !c->v || !c->v->get_state) return -1;
    int st = c->v->get_state(c);
    if (st < 0) return -1;
    *out = (sic_chg_state_t)st;
    return 0;
}
