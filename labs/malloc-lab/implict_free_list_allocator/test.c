#include "mm.h"
int main() {
  mem_init();
  mm_init();
  void* p1 = mm_malloc(2040);
  printf("%x\n", p1);
  void* p2 = mm_malloc(2040);
  printf("%x\n", p2);
  mem_deinit();
  return 0;
}
