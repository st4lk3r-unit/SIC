#pragma once
#ifdef __cplusplus
extern "C" {
#endif
struct amp_vtbl_s { void (*enable)(const void*, int on); };
typedef struct amp_s { const struct amp_vtbl_s* v; void* impl; } amp_t;
const amp_t* sic_amp(int index);
#ifdef __cplusplus
}
#endif
