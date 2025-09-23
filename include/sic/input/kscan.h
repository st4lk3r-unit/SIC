#pragma once
#ifdef __cplusplus
extern "C" {
#endif
struct kscan_vtbl_s { int (*read_key)(const void*); };
typedef struct kscan_s { const struct kscan_vtbl_s* v; void* impl; } kscan_t;
const kscan_t* sic_kbd(int index);
int kscan_read_bitmap(const struct kscan_s* self, unsigned long long* out_bitmap);
#ifdef __cplusplus
}
#endif
