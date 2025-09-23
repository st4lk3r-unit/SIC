#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Speaker */
int  sic_spk_open(int bclk_pin, int ws_pin, int dout_pin, int sample_rate_hz);
int  sic_spk_write(const int16_t* mono, size_t nframes);
void sic_spk_close(void);
int  sic_spk_beep_1khz_ms(unsigned ms);

/* Microphone (PDM) */
int  sic_mic_open(int clk_pin, int data_pin, int sample_rate_hz, int right_slot);
int  sic_mic_read(int16_t* out_frames, size_t max_frames, int timeout_ms);
void sic_mic_close(void);

/* Helpers */
size_t sic_audio_upsample16to48(const int16_t* in, size_t n_in, int16_t* out, size_t out_cap);
void   sic_audio_postprocess(int16_t* s, size_t n);

#ifdef __cplusplus
}
#endif
