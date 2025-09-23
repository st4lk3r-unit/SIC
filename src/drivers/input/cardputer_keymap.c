#include <stdint.h>
char sic_keymap_cardputer(int idx){
  static const char map[4][14] = {
    { '`','1','2','3','4','5','6','7','8','9','0','-','=', '\b' },
    { '\t','q','w','e','r','t','y','u','i','o','p','[',']','\\' },
    { 0,    0,  'a','s','d','f','g','h','j','k','l',';','\'','\n' },
    { 0,    0,  0,  'z','x','c','v','b','n','m',',','.','/',' ' }
  };
  if (idx < 0 || idx >= 56) return 0;
  int row = idx & 3;
  int col = idx >> 2;
  return map[row][col];
}
