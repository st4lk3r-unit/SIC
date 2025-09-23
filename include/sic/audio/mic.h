#pragma once
#ifdef __cplusplus
extern "C" {
#endif
struct mic_vtbl_s { int (*start)(const void*, int hz); int (*read)(const void*, short* buf, int n); };
typedef struct mic_s { const struct mic_vtbl_s* v; void* impl; } mic_t;
const mic_t* sic_mic(int index);
#ifdef __cplusplus
}
#endif
