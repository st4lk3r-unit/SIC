#include "sic/sic_board.h"
static const struct sic_board_ic_s g_ics[] = {
  { SIC_F_KSCAN, "kbd_74hc138" },
  { SIC_F_CHARGER, "tp4057" },
  { SIC_F_KSCAN, "dummy", NULL }
};
const struct sic_board_s SIC_BOARD_CARDPUTER = {
  .name = "cardputer",
  .ics = g_ics,
  .ic_count = sizeof(g_ics)/sizeof(g_ics[0])
};
