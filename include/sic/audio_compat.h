#pragma once
#include "audio.h"
#ifdef __cplusplus
extern "C" {
#endif

// MIC_SLOT_RIGHT, SIC_MIC_CLK and SIC_MIC_DATA must be defined by the board
// header or build flags — there are no generic defaults because correct values
// are board-specific (e.g. Cardputer: CLK=43, DATA=46).
#ifndef MIC_SLOT_RIGHT
#define MIC_SLOT_RIGHT 1
#endif
#if !defined(SIC_MIC_CLK) && !defined(SIC_AUDIO_NO_COMPAT)
#  error "SIC_MIC_CLK must be defined (e.g. via build_flags: -DSIC_MIC_CLK=43)"
#endif
#if !defined(SIC_MIC_DATA) && !defined(SIC_AUDIO_NO_COMPAT)
#  error "SIC_MIC_DATA must be defined (e.g. via build_flags: -DSIC_MIC_DATA=46)"
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
