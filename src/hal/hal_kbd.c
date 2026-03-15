
#include <string.h>
#include "sic/hal.h"
#include "sic/input/kscan.h"
#include "sic/bus/delay.h"

/* previous bitmap to detect edges */
static uint64_t g_prev = 0;

static int is_modifier_idx(int idx, sic_keycode_t* out_code){
  /* Map a few fixed indices to modifiers for Cardputer layout; tweak per board keymap if needed */
  switch(idx){
    case 0:  *out_code = SIC_KEY_SHIFT; return 1;
    case 1:  *out_code = SIC_KEY_CTRL;  return 1;
    case 2:  *out_code = SIC_KEY_ALT;   return 1;
    case 3:  *out_code = SIC_KEY_FN;    return 1;
    default: return 0;
  }
}


static char shiftify(char c){
  if (c>='a' && c<='z') return (char)(c-'a'+'A');
  switch(c){
    case '`': return '~';
    case '1': return '!';
    case '2': return '@';
    case '3': return '#';
    case '4': return '$';
    case '5': return '%';
    case '6': return '^';
    case '7': return '&';
    case '8': return '*';
    case '9': return '(';
    case '0': return ')';
    case '-': return '_';
    case '=': return '+';
    case '[': return '{';
    case ']': return '}';
    case '\\': return '|';
    case ';': return ':';
    case '\'': return '"';
    case ',': return '<';
    case '.': return '>';
    case '/': return '?';
    default:  return c;
  }
}

int sic_key_poll(sic_key_event_t* ev){
  if (!ev) return -1;
  const kscan_t* k = sic_kbd(0); if (!k || !k->v || !k->v->read_key) return -1;

  unsigned long long bm = 0;
  if (kscan_read_bitmap(k, &bm) != 0) return 0; /* no change */

  /* compute transitions */
  unsigned long long down = (bm & ~g_prev);
  unsigned long long up   = (~bm & g_prev);

  /* prefer down events */
  int idx = -1; int pressed = 0;
  if (down){ idx = __builtin_ctzll(down); pressed = 1; }
  else if (up){ idx = __builtin_ctzll(up); pressed = 0; }
  else return 0;

  memset(ev, 0, sizeof *ev);
  ev->pressed = (uint8_t)pressed;

  sic_keycode_t mod;
  if (is_modifier_idx(idx, &mod)){ ev->code = mod; ev->ascii = 0; }
  else {
    char c = k->keymap ? k->keymap(idx) : 0;
    ev->ascii = c;
    ev->code  = (c == 0) ? SIC_KEY_NONE : (sic_keycode_t)c;
  }

  g_prev = bm;
  return 1;
}

int  sic_readline(char* buf, int maxlen, int timeout_ms){
  if (!buf || maxlen<=1) return -1;
  int n=0;
  unsigned long t0 = sic_millis();
  for(;;){
    sic_key_event_t ev;
    if (sic_key_poll(&ev)>0 && ev.pressed){
      if (ev.code==SIC_KEY_ENTER){ buf[n]=0; return n; }
      else if (ev.code==SIC_KEY_BACKSPACE){ if (n>0) n--; }
      else if (ev.ascii && n<maxlen-1){ buf[n++]=ev.ascii; }
    }
    if (timeout_ms>0 && (int)(sic_millis()-t0) > timeout_ms){ buf[n]=0; return n; }
    sic_delay_ms(1);
  }
}
