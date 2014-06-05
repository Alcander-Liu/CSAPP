#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
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

/* Obtain a backtrace and print it to stdout. */
void print_trace (void) {
  void *array[10];
  size_t size;
  char **strings;
  size_t i;

  size = backtrace (array, 10);
  strings = backtrace_symbols (array, size);

  printf ("Obtained %zd stack frames.\n", size);

  for (i = 0; i < size; i++)
    printf ("%s\n", strings[i]);
    free (strings);
  }

  /* A dummy function to make the backtrace more interesting. */
void dummy_function (void) {
  print_trace ();
}

int main() {
  dummy_function();
  // mem_init();
  // mm_init();
  // Test();
  // mem_deinit();
  return 0;
}