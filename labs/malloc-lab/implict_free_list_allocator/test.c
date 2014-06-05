#include "mm.h"
extern void show_heap(void);
#define DebugStr(args...) fprintf(stderr, args);

void Test() {
  void *p0 = mm_malloc(512);
  DebugStr("mm_malloc: p0 512\n");
  show_heap();

  void *p1 = mm_malloc(128);
  DebugStr("mm_malloc: p1 128\n");
  show_heap();

  p0 = mm_realloc(p0, 640);
  DebugStr("mm_realloc: p0 640\n");
  show_heap();
}

int main() {
  mem_init();
  mm_init();
  Test();
  mem_deinit();
  return 0;
}