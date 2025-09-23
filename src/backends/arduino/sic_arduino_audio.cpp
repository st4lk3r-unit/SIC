#define SIC_AUDIO_NO_COMPAT 1
#define SIC_AUDIO_NO_COMPAT 1
#include <Arduino.h>
#include <driver/i2s.h>
extern "C" {
  #include "sic/sic.h"    // includes sic_audio.h indirectly
}

#ifndef SIC_AUDIO_DMA_LEN
#define SIC_AUDIO_DMA_LEN   256
#endif
#ifndef SIC_AUDIO_DMA_CNT
#define SIC_AUDIO_DMA_CNT   6
#endif

static bool g_spk=false, g_mic=false;
static int  g_spk_sr = 48000;
static int  g_mic_sr = 16000;

int sic_spk_open(int bclk_pin, int ws_pin, int dout_pin, int sample_rate_hz){
  if (g_spk) return 0;
  g_spk_sr = sample_rate_hz;

  i2s_config_t cfg = {};
  cfg.mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate          = sample_rate_hz;
  cfg.bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.dma_buf_len          = SIC_AUDIO_DMA_LEN;
  cfg.dma_buf_count        = SIC_AUDIO_DMA_CNT;
  cfg.use_apll             = false;
  cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;

  if (i2s_driver_install(I2S_NUM_1, &cfg, 0, nullptr) != ESP_OK) return -1;

  i2s_pin_config_t pins = {};
  pins.mck_io_num   = I2S_PIN_NO_CHANGE;
  pins.bck_io_num   = bclk_pin;
  pins.ws_io_num    = ws_pin;
  pins.data_out_num = dout_pin;
  pins.data_in_num  = I2S_PIN_NO_CHANGE;

  if (i2s_set_pin(I2S_NUM_1, &pins) != ESP_OK){ i2s_driver_uninstall(I2S_NUM_1); return -2; }
  i2s_set_clk(I2S_NUM_1, sample_rate_hz, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
  g_spk = true;
  return 0;
}

int sic_spk_write(const int16_t* mono, size_t nframes){
  if (!g_spk || !mono || !nframes) return 0;
  size_t written_frames = 0;
  static int16_t stereo[512*2];
  while (nframes){
    size_t chunk = nframes > 512 ? 512 : nframes;
    for (size_t i=0;i<chunk;i++){ stereo[2*i] = mono[i]; stereo[2*i+1] = mono[i]; }
    size_t bytes=0;
    if (i2s_write(I2S_NUM_1, stereo, chunk*2*sizeof(int16_t), &bytes, portMAX_DELAY) != ESP_OK) break;
    written_frames += bytes/(2*sizeof(int16_t));
    mono    += chunk;
    nframes -= chunk;
  }
  return (int)written_frames;
}

void sic_spk_close(void){
  if (!g_spk) return;
  i2s_driver_uninstall(I2S_NUM_1);
  g_spk=false;
}

int sic_spk_beep_1khz_ms(unsigned ms){
  if (g_mic){ i2s_driver_uninstall(I2S_NUM_0); g_mic=false; }
  if (sic_spk_open(
        #ifdef SIC_SPK_BCLK
          SIC_SPK_BCLK,
        #else
          -1,
        #endif
        #ifdef SIC_SPK_WS
          SIC_SPK_WS,
        #else
          -1,
        #endif
        #ifdef SIC_SPK_DOUT
          SIC_SPK_DOUT,
        #else
          -1,
        #endif
        g_spk_sr) != 0) return -1;

  const size_t N = 512;
  static int16_t buf[N];
  float w = 2.0f*PI*1000.0f/(float)g_spk_sr;
  uint32_t total = (uint64_t)g_spk_sr*ms/1000;
  while (total){
    size_t n = (total>N)? N:total;
    for (size_t i=0;i<n;i++) buf[i] = (int16_t)(sinf(w*i)*12000);
    sic_spk_write(buf, n);
    total -= n;
  }
  return 0;
}

int sic_mic_open(int clk_pin, int data_pin, int sample_rate_hz, int right_slot){
  if (g_mic) return 0;
  if (g_spk){ i2s_driver_uninstall(I2S_NUM_1); g_spk=false; }
  g_mic_sr = sample_rate_hz;

  i2s_config_t cfg = {};
  cfg.mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
  cfg.sample_rate          = sample_rate_hz;
  cfg.bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format       = right_slot ? I2S_CHANNEL_FMT_ONLY_RIGHT : I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.dma_buf_len          = SIC_AUDIO_DMA_LEN;
  cfg.dma_buf_count        = SIC_AUDIO_DMA_CNT;
  cfg.use_apll             = false;
  cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;

  if (i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr) != ESP_OK) return -1;

  i2s_pin_config_t pins = {};
  pins.mck_io_num   = I2S_PIN_NO_CHANGE;
  pins.bck_io_num   = I2S_PIN_NO_CHANGE; // PDM has no BCK
  pins.ws_io_num    = clk_pin;           // PDM clock on WS
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num  = data_pin;

  if (i2s_set_pin(I2S_NUM_0, &pins) != ESP_OK){ i2s_driver_uninstall(I2S_NUM_0); return -2; }
  i2s_set_clk(I2S_NUM_0, sample_rate_hz, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
  g_mic = true;
  return 0;
}

int sic_mic_read(int16_t* out_frames, size_t max_frames, int timeout_ms){
  if (!g_mic || !out_frames || !max_frames) return 0;
  size_t got = 0;
  if (i2s_read(I2S_NUM_0, out_frames, max_frames*sizeof(int16_t), &got, timeout_ms/portTICK_PERIOD_MS) != ESP_OK) return 0;
  return (int)(got/sizeof(int16_t));
}

void sic_mic_close(void){
  if (!g_mic) return;
  i2s_driver_uninstall(I2S_NUM_0);
  g_mic=false;
}
