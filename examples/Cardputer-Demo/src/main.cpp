#include <Arduino.h>
#include <Wire.h>
extern "C" {
  #include "sic/sic.h"
}

// ---------- Pins (already provided by your ini/variants) ----------
#ifndef SIC_SPK_BCLK
#define SIC_SPK_BCLK 41
#endif
#ifndef SIC_SPK_WS
#define SIC_SPK_WS   43
#endif
#ifndef SIC_SPK_DOUT
#define SIC_SPK_DOUT 42
#endif
#ifndef BAT_ADC_PIN
#define BAT_ADC_PIN 10
#endif
#ifndef BAT_DIV_K
#define BAT_DIV_K 2.0f
#endif
#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN 41
#endif
#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN 42
#endif

// Weak no-ops to avoid link errors if HAL doesn’t provide them
extern "C" __attribute__((weak)) int  sic_spk_drain(void){ return 0; }

// ---------- Audio rates & buffers ----------
static const int SR_MIC = 16000;                // request 16 kHz from HAL
static const int SR_SPK = 48000;                // play at 48 kHz
static const size_t IO_CHUNK = 512;             // frames per mic read

static int16_t micbuf[IO_CHUNK];                // MONO 16 kHz
static int16_t upbuf[IO_CHUNK * 3];             // 48 kHz (×3)

enum mode_e { MODE_IDLE=0, MODE_VU, MODE_RECPLAY };
static mode_e g_mode = MODE_IDLE;
static bool   g_phase_record = true;

static const int    REC_SECONDS = 3;
static const size_t REC_FRAMES  = (size_t)SR_MIC * (size_t)REC_SECONDS; // mono 16 kHz
static int16_t recbuf[REC_FRAMES];
static size_t  rec_fill = 0;

// ---------- Helpers ----------
static float vu_level_mono(const int16_t* s, int n){
  if (n <= 0) return 0.f;
  int64_t sum = 0; for (int i=0;i<n;i++) sum += s[i];
  float mean = (float)sum / (float)n;
  double acc = 0.0;
  for (int i=0;i<n;i++){ float v = (float)s[i] - mean; acc += (double)v*v; }
  float rms = sqrtf((float)(acc / (double)n));
  float vu  = rms / 2048.0f; if (vu < 0) vu = 0; if (vu > 1) vu = 1;
  return vu;
}

// 3× linear upsampler 16k -> 48k (mono)
static size_t upsample16k_to_48k_linear(const int16_t* in, size_t n_in, int16_t* out, size_t out_cap){
  if (!in || !out || n_in==0) return 0;
  size_t need = n_in * 3; if (out_cap < need) n_in = out_cap / 3;
  for (size_t i=0;i+1<n_in;i++){
    int32_t a = in[i], b = in[i+1], d = b - a;
    out[3*i+0] = (int16_t)a;
    out[3*i+1] = (int16_t)(a + d/3);
    out[3*i+2] = (int16_t)(a + (2*d)/3);
  }
  out[3*(n_in-1) + 0] = in[n_in-1];
  out[3*(n_in-1) + 1] = in[n_in-1];
  out[3*(n_in-1) + 2] = in[n_in-1];
  return n_in * 3;
}

static void stop_all(){
  sic_mic_close();
  sic_spk_close();
  g_mode = MODE_IDLE;
  Serial.println(F("[MODE] stopped"));
}

static void sys_info(){
  sic_sysinfo_t si{};
  if (sic_sysinfo(&si)==0){
    char mac[18]; snprintf(mac, sizeof mac, "%02X:%02X:%02X:%02X:%02X:%02X",
      si.mac[0],si.mac[1],si.mac[2],si.mac[3],si.mac[4],si.mac[5]);
    Serial.printf("[SYS] model=%s rev=%u CPU=%uMHz flash=%uB@%uHz psram=%uB mac=%s\n",
      si.chip_model, si.chip_rev, si.cpu_mhz, si.flash_bytes, si.flash_hz, si.psram_bytes, mac);
  }
  Serial.printf("[CFG] I2C_SDA_PIN=%d I2C_SCL_PIN=%d  MIC_CLK=%d MIC_DATA=%d  SPK_BCLK=%d WS=%d DOUT=%d\n",
    I2C_SDA_PIN, I2C_SCL_PIN, SIC_MIC_CLK, SIC_MIC_DATA, SIC_SPK_BCLK, SIC_SPK_WS, SIC_SPK_DOUT);
#ifdef BAT_ADC_PIN
  Serial.printf("[CFG] BAT_ADC_PIN=%d BAT_DIV_K=%.2f\n", BAT_ADC_PIN, (double)BAT_DIV_K);
#endif
}

static void i2c_scan(){
  Serial.printf("[I2C] scanning SDA=%d SCL=%d...\n", I2C_SDA_PIN, I2C_SCL_PIN);
  uint8_t found=0;
  for (uint8_t a=1;a<127;++a){
    Wire.beginTransmission(a);
    if (Wire.endTransmission()==0){ Serial.printf("  - 0x%02X\n", a); found++; }
  }
  if (!found) Serial.println("[I2C] none");
}

static void adc_read_once(){
#ifdef BAT_ADC_PIN
  analogReadResolution(12);
  const int N=8; uint32_t acc=0;
  for (int i=0;i<N;i++){ acc += (uint32_t)analogRead(BAT_ADC_PIN); delayMicroseconds(200); }
  float raw = (float)acc / (float)N;
  float mv = raw * (3300.0f/4095.0f) * BAT_DIV_K;
  Serial.printf("[BAT] pin=%d ~%.3f V\n", BAT_ADC_PIN, mv/1000.0f);
#else
  Serial.println("[BAT] ADC pin not defined");
#endif
}

// Software beep @1 kHz so it works even if HAL beep is a stub
static void play_beep_ms(unsigned ms, int freq_hz = 1000, int amp = 2200){
  if (sic_spk_open(SIC_SPK_BCLK, SIC_SPK_WS, SIC_SPK_DOUT, SR_SPK)!=0){
    Serial.println(F("[ERR] spk open (beep)")); return;
  }
  const float kTwoPi = 6.28318530718f;
  const float dt = 1.0f / (float)SR_SPK;
  const size_t CH = 256; static int16_t sbuf[CH];
  float phase=0.f, dphi = kTwoPi * (float)freq_hz * dt;
  size_t total = (size_t)((ms/1000.0f)*SR_SPK), done=0;
  while (done < total){
    size_t n = min(CH, total-done);
    for (size_t i=0;i<n;i++){
      sbuf[i] = (int16_t)(amp * sinf(phase));
      phase += dphi; if (phase > kTwoPi) phase -= kTwoPi;
    }
    sic_spk_write(sbuf, n); done += n;
  }
  sic_spk_drain(); sic_spk_close();
}

// ---------- Input: ALWAYS echo like original main ----------
static bool read_serial_key(char* out){
  if (Serial.available()){ *out = (char)Serial.read(); Serial.printf("[KEY] '%c'\n", *out); return true; }
  return false;
}
static bool read_kbd_key(char* out){
  sic_key_event_t ev{};
  if (sic_key_poll(&ev)>0 && ev.pressed){
    if (ev.ascii){ *out = ev.ascii; Serial.printf("[KEY] '%c'\n", *out); }
    else { *out = 0; Serial.printf("[KEY] code=%u mods[S=%d C=%d A=%d O=%d F=%d Caps=%d]\n",
              (unsigned)ev.code, ev.shift, ev.ctrl, ev.alt, ev.opt, ev.fn, ev.caps); }
    return true;
  }
  return false;
}

void setup(){
  delay(1500);
  Serial.begin(115200);
  unsigned long t0=millis(); while (!Serial && (millis()-t0)<2000) delay(10);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  sic_begin_opts_t opts{ .init_buses = 1, .lazy_drivers = 0 };
#ifdef SIC_USE_BOARD_CARDPUTER
  int rc = sic_begin(&SIC_BOARD_CARDPUTER, &opts);
#else
  int rc = sic_begin(nullptr, &opts);
#endif
  Serial.printf("[SIC] sic_begin rc=%d\n", rc);

  // ---- Boot diagnostics: prove keyboard driver is registered ----
  int kcnt = sic_count_fn(SIC_F_KSCAN);
  Serial.printf("[KBD] registered drivers: %d\n", kcnt);
  for (int i=0;i<kcnt;i++){
    const char* nm = sic_name_fn(SIC_F_KSCAN, i);
    Serial.printf("  - kscan[%d] = %s\n", i, nm ? nm : "(null)");
  }
  const kscan_t* k = sic_kbd(0);
  if (!k) Serial.println("[KBD] sic_kbd(0) = NULL (driver not probed)");
  else {
    unsigned long long bm=0;
    if (kscan_read_bitmap(k, &bm)==0) Serial.printf("[KBD] initial bitmap: 0x%016llX\n", bm);
    else Serial.println("[KBD] kscan_read_bitmap failed");
  }

  sys_info();
  Serial.println(F("[KEYS] 'b'=beep  'm'=mic VU  'l'=rec 3s->play(loop)  's'=stop  'a'=adc  'i'=i2c"));
}

// Raw bitmap watcher (helps verify scanning even if events don’t map yet)
static void kbd_bitmap_watchdog(){
  static unsigned long long last_bm = 0ULL;
  static unsigned long last_t = 0;
  if (millis() - last_t < 50) return; // throttle
  last_t = millis();
  const kscan_t* k = sic_kbd(0);
  if (!k) return;
  unsigned long long bm=0;
  if (kscan_read_bitmap(k, &bm)==0 && bm != last_bm){
    Serial.printf("[KBD] bm: 0x%016llX -> 0x%016llX\n", last_bm, bm);
    last_bm = bm;
  }
}

static void handle_command(char c){
  switch(c){
    case 'b': Serial.println(F("[CMD] beep")); play_beep_ms(300); break;
    case 'm':
      Serial.println(F("[CMD] VU"));
      if (sic_mic_start(SR_MIC)==0){ g_mode=MODE_VU; Serial.println(F("[MODE] MIC VU")); }
      else Serial.println(F("[ERR] mic open"));
      break;
    case 'l':
      Serial.println(F("[CMD] rec/play"));
      rec_fill=0; g_phase_record=true;
      if (sic_mic_start(SR_MIC)==0){ g_mode=MODE_RECPLAY; Serial.println(F("[MODE] REC 3s -> PLAY (loop)")); }
      else Serial.println(F("[ERR] mic open"));
      break;
    case 's': Serial.println(F("[CMD] stop")); stop_all(); break;
    case 'a': Serial.println(F("[CMD] adc")); adc_read_once(); break;
    case 'i': Serial.println(F("[CMD] i2c")); i2c_scan(); break;
    default: break; // non-commands already echoed
  }
}

void loop(){
  // Read from Serial first, then keyboard; always echo like original
  char c;
  if (read_serial_key(&c)) handle_command(c);
  if (read_kbd_key(&c))    if (c) handle_command(c); // ASCII commands execute after echo

  // Also log raw matrix changes (helps bring-up)
  kbd_bitmap_watchdog();

  // Modes
  if (g_mode==MODE_VU){
    int n = sic_mic_read(micbuf, IO_CHUNK);       // compat 2-arg
    if (n > 0) Serial.printf("[VU] %.3f\n", vu_level_mono(micbuf, n));
  }
  else if (g_mode==MODE_RECPLAY){
    if (g_phase_record){
      int n = sic_mic_read(micbuf, IO_CHUNK);     // mono @16 kHz
      if (n > 0){
        size_t tocopy = min((size_t)n, REC_FRAMES - rec_fill);
        memcpy(&recbuf[rec_fill], micbuf, tocopy * sizeof(int16_t));
        rec_fill += tocopy;
        if (rec_fill >= REC_FRAMES){
          sic_mic_close();
          sic_audio_postprocess(recbuf, REC_FRAMES); // no-op if not implemented
          if (sic_spk_open(SIC_SPK_BCLK, SIC_SPK_WS, SIC_SPK_DOUT, SR_SPK)!=0){
            Serial.println(F("[ERR] spk open")); g_mode=MODE_IDLE;
          } else {
            g_phase_record=false; Serial.println(F("[PLAY] playing 3s"));
          }
        }
      }
    } else {
      size_t played=0;
      while (played < REC_FRAMES){
        size_t n_in  = min(IO_CHUNK, REC_FRAMES - played);
        size_t n_out = upsample16k_to_48k_linear(&recbuf[played], n_in, upbuf, sizeof(upbuf)/sizeof(upbuf[0]));
        if (n_out) sic_spk_write(upbuf, n_out);
        played += n_in;
        delay(1);
      }
      sic_spk_drain(); sic_spk_close();
      rec_fill=0;
      if (sic_mic_start(SR_MIC)!=0){ g_mode=MODE_IDLE; Serial.println(F("[ERR] mic re-start")); }
      else { g_phase_record=true; Serial.println(F("[REC] recording 3s")); }
    }
  }

  sic_delay_ms(1);
}
