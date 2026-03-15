#pragma once
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Board-supplied keyboard configuration passed via sic_board_ic_t.cfg.
 * Set keymap to a function that maps a raw bitmap index → ASCII character,
 * or NULL if this board has no ASCII keymap.
 */
/* Special character emitted by keymap_alt to signal a caps-lock toggle.
 * The application (e.g. neu_kbd_poll) intercepts this and must NOT push it
 * to the input buffer. */
#define SIC_KEY_CAPS_LOCK '\x0E'

typedef struct sic_kscan_cfg_s {
    char (*keymap)(int idx);         /* base layer: index → ASCII, NULL = none */
    char (*keymap_alt)(int idx);     /* alt/orange layer (NULL = no alt layer) */
    unsigned long long modifier_mask;/* bitmask of orange/alt modifier bit(s); consumed, not emitted */
    unsigned long long shift_mask;   /* bitmask of shift key bit(s); consumed, applies uppercase */
} sic_kscan_cfg_t;

struct kscan_s; /* forward declaration — defined below */

struct kscan_vtbl_s {
    int (*read_key)(const void*);
    /* read_bitmap: fill *out with the current key-down bitmask.
     * Bit N = key at index N is pressed.  Returns 0 on success, <0 on error.
     * Drivers MUST set this — it is the primary scan path used by hal_kbd. */
    int (*read_bitmap)(const struct kscan_s* self, unsigned long long* out);
};

typedef struct kscan_s {
    const struct kscan_vtbl_s* v;
    void* impl;
    char (*keymap)(int idx);         /* base layer */
    char (*keymap_alt)(int idx);     /* alt/orange layer (NULL = none) */
    unsigned long long modifier_mask;/* orange/alt modifier bits — consumed, not emitted */
    unsigned long long shift_mask;   /* shift key bits — consumed, applies uppercase */
} kscan_t;

const kscan_t* sic_kbd(int index);
/* Dispatches to self->v->read_bitmap — defined in kscan_dispatch.c */
int kscan_read_bitmap(const struct kscan_s* self, unsigned long long* out_bitmap);

#ifdef __cplusplus
}
#endif
