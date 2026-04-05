#include "sic/sic_board.h"
#include "sic/input/kscan.h"

/* Defined in src/drivers/input/cardputer_keymap.c */
extern char sic_keymap_cardputer(int idx);

/* Bit 3 = SHIFT key (row 3, col 0 in keymap), bit 2 = FN/Alt key (row 2, col 0) */
static const sic_kscan_cfg_t g_kbd_cfg = {
    .keymap        = sic_keymap_cardputer,
    .shift_mask    = (1ULL << 3),
    .modifier_mask = (1ULL << 2),
};

static const struct sic_board_ic_s g_ics[] = {
    { SIC_F_KSCAN,   "kbd_74hc138", &g_kbd_cfg },
    { SIC_F_CHARGER, "tp4057",      NULL       },
};

const struct sic_board_s SIC_BOARD_CARDPUTER = {
    .name     = "cardputer",
    .ics      = g_ics,
    .ic_count = sizeof(g_ics) / sizeof(g_ics[0])
};
