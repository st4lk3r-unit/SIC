
#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>
#include <math.h>
extern "C" {
  #include "sic/hal.h"
  #include "sic/sic.h"
  #include "sic/sic_board.h"
  #include "sic/bus/delay.h"
  #include "sic/input/kscan.h"
  #include "sic/power/switch.h"
  #include "sic/power/charger.h"
  #include "sic/audio/amp.h"
  #include "sic/audio/mic.h"
  #include "sic/ir/ir_tx.h"
  #include "sic/storage/sd.h"
  #include "sic/bus/i2c_bus.h"
  #include "sic/sic_registry.h"
}

static int g_cap_present[SIC_CAP_COUNT] = {0};
static int g_cap_counts[SIC_CAP_COUNT]  = {0};

static void refresh_caps(){
  g_cap_counts[SIC_CAP_PWR_SW]   = (sic_pwr_sw(0)!=nullptr);
  g_cap_counts[SIC_CAP_AUDIO_AMP]= (sic_amp(0)!=nullptr);
  g_cap_counts[SIC_CAP_MIC]      = (sic_mic(0)!=nullptr);
  g_cap_counts[SIC_CAP_CHARGER]  = (sic_charger(0)!=nullptr);
  g_cap_counts[SIC_CAP_KSCAN]    = (sic_kbd(0)!=nullptr);
  g_cap_counts[SIC_CAP_IR_TX]    = (sic_ir_tx(0)!=nullptr);
  g_cap_counts[SIC_CAP_SD]       = (sic_sd(0)!=nullptr);

  for (int i=0;i<SIC_CAP_COUNT;i++) g_cap_present[i] = g_cap_counts[i] > 0;
}

extern "C" {
/* C API impl */
int  sic_begin(const void* board_desc, const sic_begin_opts_t* opts){
  (void)opts;
  /* legacy path: call legacy begin which registers known drivers and probes board */
  int rc = sic_begin_legacy((const sic_board_t*)board_desc, opts);
  refresh_caps();
  return rc;
}

void sic_end(void){ /* no-op for Arduino demo */ }

int  sic_sysinfo(sic_sysinfo_t* out){
  if (!out) return -1;
  out->chip_model = "ESP32";
  out->chip_rev   = 0;
  out->cpu_mhz    = (uint32_t) (F_CPU/1000000ul);
  out->flash_bytes= 8*1024*1024u;
  out->flash_hz   = 80000000u;
  out->psram_bytes= 0;
  uint8_t mac[6];
  if (WiFi.getMode() == WIFI_OFF) WiFi.mode(WIFI_MODE_STA);
  WiFi.macAddress(mac);
  if (mac[0]==0 && mac[1]==0 && mac[2]==0 && mac[3]==0 && mac[4]==0 && mac[5]==0) {
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
  }
  for (int i=0;i<6;i++) out->mac[i]=mac[i];
  return 0;
}

int  sic_has(sic_cap_t cap){ return (cap>=0 && cap<SIC_CAP_COUNT) ? g_cap_present[cap] : 0; }
int  sic_count_cap(sic_cap_t cap){ return (cap>=0 && cap<SIC_CAP_COUNT) ? g_cap_counts[cap] : 0; }

static int hal_amp_enable_stub(int on){
  const amp_t* a = sic_amp(0); if (!a || !a->v || !a->v->enable) return -1;
  a->v->enable(a, on); return 0;
}

/* removed legacy sic_mic_start wrapper */

/* removed legacy sic_mic_read wrapper */

int  sic_ir_send_nec(uint32_t code){
  const ir_t* ir = sic_ir_tx(0); if (!ir || !ir->v || !ir->v->send_nec) return -1;
  ir->v->send_nec(ir, code); return 0;
}

int  sic_sd_present(void){ return sic_sd(0)!=NULL; }

int  sic_i2c_begin(int sda, int scl, uint32_t hz){ return sic_i2c_begin_bus(0, sda, scl, hz); }
int  sic_i2c_scan(uint8_t* addrs, int max){ return sic_i2c_scan_bus(0, addrs, max); }



int sic_charger_state(sic_chg_state_t* out){
  if (!out) return -1;
  const charger_t* c = sic_charger(0); if (!c || !c->v || !c->v->get_state) return -1;
  int st = c->v->get_state(c);
  if (st < 0) return -1;
  *out = (sic_chg_state_t)st;
  return 0;
}
}
