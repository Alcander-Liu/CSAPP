#include "mm.h"
extern void show_heap(void);
#define DebugStr(args...) fprintf(stderr, args);
int main() {
  mem_init();
  mm_init();
  DebugStr("malloc p0 2040\n");
  void *p0 = mm_malloc(2040);
  show_heap();
  getchar();

  DebugStr("malloc p1 2040\n");
  void *p1 = mm_malloc(2040);
  show_heap();
  getchar();

  DebugStr("free p1 2040\n");
  mm_free(p1);
  show_heap();
  getchar();

  DebugStr("malloc p2 48\n");
  void *p2 = mm_malloc(48);
  show_heap();
  getchar();

  DebugStr("malloc p3 4072\n");
  void *p3 = mm_malloc(4072);
  show_heap();
  getchar();

  DebugStr("free p3 4072\n");
  mm_free(p3);
  show_heap();
  getchar();

  DebugStr("malloc p4 4072\n");
  void *p4 = mm_malloc(4072);
  show_heap();
  getchar();

  DebugStr("free p0 2040\n");
  mm_free(p0);
  show_heap();
  getchar();

  DebugStr("free p2 48\n");
  mm_free(p2);
  show_heap();
  getchar();

  DebugStr("malloc p5 4072\n");
  void *p5 = mm_malloc(4072);
  show_heap();
  getchar();

  DebugStr("free p4 4072\n");
  mm_free(p4);
  show_heap();
  getchar();

  DebugStr("free p5 4072\n");
  mm_free(p5);
  show_heap();
  getchar();
  mem_deinit();
  return 0;
}