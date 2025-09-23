
#include <stdlib.h>
#include <string.h>
#include "sic/sic.h"
#include "sic/sic_registry.h"
#include "sic/input/kscan.h"
#include "sic/audio/mic.h"
#include "sic/audio/amp.h"
#include "sic/power/switch.h"
#include "sic/power/charger.h"
#include "sic/storage/sd.h"
#include "sic/ir/ir_tx.h"

typedef struct item_s { const char* name; void* inst; struct item_s* next; } item_t;
static item_t* g_items[SIC_F__COUNT] = {0};

static const sic_driver_t* g_drivers[64]; static int g_ndrv = 0;

void  sic_registry_register(const sic_driver_t* drv){
  if (g_ndrv < (int)(sizeof g_drivers/sizeof g_drivers[0])) g_drivers[g_ndrv++] = drv;
}

int   sic_registry_add(sic_func_id_t fn, const char* name, void* inst){
  item_t* it = (item_t*)calloc(1,sizeof *it); if (!it) return -1;
  it->name=name; it->inst=inst; it->next=g_items[fn]; g_items[fn]=it;
  return 0;
}

int   sic_count_fn(sic_func_id_t fn){
  int n=0; for (item_t* it=g_items[fn]; it; it=it->next) n++; return n;
}

const char* sic_name_fn(sic_func_id_t fn, int idx){
  for (item_t* it=g_items[fn]; it; it=it->next){ if (idx==0) return it->name; idx--; }
  return 0;
}

void* sic_get_fn(sic_func_id_t fn, int idx){
  for (item_t* it=g_items[fn]; it; it=it->next){ if (idx==0) return it->inst; idx--; }
  return 0;
}

/* Built-in driver registrations */
extern void sic_register_driver_kbd_74hc138(void);
extern void sic_register_driver_dummy(void);
extern void sic_register_driver_tp4057(void);

int sic_begin_legacy(const sic_board_t* board, const sic_begin_opts_t* opts){
  (void)opts;
  sic_register_driver_kbd_74hc138();
  sic_register_driver_tp4057();
  sic_register_driver_dummy();

  if (!board) return 0;

  for (int i=0;i<board->ic_count;i++){
    const sic_board_ic_t* ic = &board->ics[i];
    for (int d=0; d<g_ndrv; d++){
      const sic_driver_t* drv = g_drivers[d];
      if (drv->function == ic->function){
        void* inst=NULL;
        if (drv->probe(ic, &inst)==0 && inst){ sic_registry_add(ic->function, drv->name, inst); break; }
      }
    }
  }
  return 1;
}

/* typed accessors */
const kscan_t*   sic_kbd(int index){ return (const kscan_t*)  sic_get_fn(SIC_F_KSCAN, index); }
const mic_t*     sic_mic(int index){ return (const mic_t*)    sic_get_fn(SIC_F_MIC, index); }
const amp_t*     sic_amp(int index){ return (const amp_t*)    sic_get_fn(SIC_F_AMP, index); }
const pwr_sw_t*  sic_pwr_sw(int index){ return (const pwr_sw_t*) sic_get_fn(SIC_F_PWR_SW, index); }
const charger_t* sic_charger(int index){ return (const charger_t*) sic_get_fn(SIC_F_CHARGER, index); }
const ir_t*      sic_ir_tx(int index){ return (const ir_t*)   sic_get_fn(SIC_F_IR_TX, index); }
const sd_t*      sic_sd(int index){ return (const sd_t*)      sic_get_fn(SIC_F_SD, index); }
