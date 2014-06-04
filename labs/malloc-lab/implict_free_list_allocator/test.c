#include "mm.h"
#define DebugStr(args...) fprintf(stderr, args);
extern void pointer_macro_test(void);
int main() {
  pointer_macro_test();
  ToBinaryStr(0xF01, 1);
  ToHexStr(0xF01, 1);
  return 0;
}