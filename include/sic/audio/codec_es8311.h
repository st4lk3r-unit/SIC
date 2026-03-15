#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Configuration passed via sic_board_ic_t.cfg for both SIC_F_MIC and SIC_F_AMP
 * entries that use the "codec_es8311" driver hint.
 */
typedef struct sic_es8311_cfg_s {
    int     i2c_bus;    /* I2C bus index (0 = primary)          */
    uint8_t i2c_addr;   /* I2C address — 0x18 for ES8311        */
    int     pin_mclk;   /* MCLK GPIO                            */
    int     pin_bclk;   /* BCLK GPIO                            */
    int     pin_ws;     /* WS / LRCK GPIO                       */
    int     pin_dout;   /* DOUT GPIO (DAC out → speaker)        */
    int     pin_din;    /* DIN  GPIO (ADC in  ← mic)            */
} sic_es8311_cfg_t;

#ifdef __cplusplus
}
#endif
