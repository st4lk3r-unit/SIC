#pragma once
#include "audio.h"
#ifdef __cplusplus
extern "C" {
#endif

// Default pins/slot if not provided by build flags
#ifndef MIC_SLOT_RIGHT
#define MIC_SLOT_RIGHT 1
#endif
#ifndef SIC_MIC_CLK
#define SIC_MIC_CLK 43
#endif
#ifndef SIC_MIC_DATA
#define SIC_MIC_DATA 46
#endif

// Compatibility shims for legacy 2-arg mic API and amp control.
// Guarded so backend implementations can opt-out and expose the real symbols.
#ifndef SIC_AUDIO_NO_COMPAT
#define sic_mic_start(hz) sic_mic_open(SIC_MIC_CLK, SIC_MIC_DATA, (hz), MIC_SLOT_RIGHT)

int sic_mic_read_compat(int16_t* pcm, int n);
#define sic_mic_read(pcm, n) sic_mic_read_compat((pcm), (n))

// No-op by default; replace with real amp driver if available.
int sic_amp_enable(int on);
#endif

#ifdef __cplusplus
}
#endif
