#pragma once
#ifdef __cplusplus
extern "C" {
#endif
struct ir_vtbl_s { void (*send_nec)(const void*, unsigned long code); };
typedef struct ir_s { const struct ir_vtbl_s* v; void* impl; } ir_t;
const ir_t* sic_ir_tx(int index);
#ifdef __cplusplus
}
#endif
