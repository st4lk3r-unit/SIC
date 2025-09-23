#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Driver contracts */
typedef int (*sic_probe_fn)(const void* board_ic, void** out_inst);
typedef void (*sic_remove_fn)(void* inst);

typedef enum { 
  SIC_F_KSCAN, SIC_F_MIC, SIC_F_AMP, SIC_F_PWR_SW, SIC_F_CHARGER, SIC_F_IR_TX, SIC_F_SD, 
  SIC_F__COUNT 
} sic_func_id_t;
const char*    sic_func_name(sic_func_id_t);
sic_func_id_t  sic_func_id_from_name(const char*);

/* Registry-visible driver descriptor */
typedef struct sic_driver_s {
  const char*   name;         /* "tp4057" */
  sic_func_id_t function;     /* enum id */
  sic_probe_fn  probe;
  sic_remove_fn remove;
} sic_driver_t;

/* Board description */
typedef struct sic_board_ic_s { sic_func_id_t function; const char* hint; const void* cfg; } sic_board_ic_t;
typedef struct sic_board_s { const char* name; const sic_board_ic_t* ics; int ic_count; } sic_board_t;

typedef enum { SIC_OK=0, SIC_ENOENT=-2, SIC_EINVAL=-3, SIC_EIO=-5, SIC_ENOMEM=-12 } sic_err_t;

#ifndef SIC_NOPIN
#define SIC_NOPIN 0xFF
#endif

#include "sic/hal.h"
#include "audio.h"
#include "audio_compat.h"
#include "input/kscan.h"
#include "power/battery.h"
#include "power/charger.h"
#include "sic/sic_board.h"        // SIC_BOARD_CARDPUTER
#include "sic/sic_registry.h"     // sic_count_fn, sic_name_fn, sic_get_fn
#include "sic/input/kscan.h"      // kscan_t, kscan_read_bitmap, sic_kbd()
#include "sic/audio_compat.h"     // 2-arg sic_mic_read(...)

#ifdef __cplusplus
}
#endif