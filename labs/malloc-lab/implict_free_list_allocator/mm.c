/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <execinfo.h>

#include "mm.h"
#include "memlib.h"

#define LOG_DEBUG_STR
#ifdef LOG_DEBUG_STR
 #define DebugStr(args...) fprintf(stderr, args);
#else
 #define DebugStr(args...)
#endif

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "xrandom",
    /* First member's full name */
    "xrandom",
    /* First member's email address */
    "xrandom@ab.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

// #ifdef __x86_64__
// #warning "64"
// #else
// #warning "32"
// #endif

#define NUM_STACK_TRACE    20
/* single word (4) or double word (8) alignment */
#define ALIGNMENT      8
#define MIN_BK_SIZE    8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define IS_ALIGN(size)      (!(size & (ALIGNMENT - 1)))

#define IS_ALIGN_WITH_MIN_BK_SIZE(size)        (!(size & (MIN_BK_SIZE-1)))

// if complie with -m32, sizeof(size_t) = 4
// if complie with -m64, sizeof(size_t) = 8
// but in either case, ALIGN(sizeof(size_t)) = 8
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE                           4
#define DSIZE                           8
#define CHUNKSIZE                       (1 << 12)               // equal the page size 4kb
// get multiple of CHUNKSIZE
#define ALIGN_CHUNKSIZE(size)           (((size) + (CHUNKSIZE - 1)) & ~(CHUNKSIZE-1));

#define READ_WORD(p)                                (*(uint32_t*)(p))
#define WRITE_WORD(p, val)                          (*(uint32_t*)(p) = (val))
// Macro Operations
// Allocated Memory Structure
// [ 1 W Header | Payload | [Optional Padding] | 1 W Footer]
// hdrp (header pointer) points to the start address of header
// ftrp (footer pointer) points to the start address of footer
// pldp (payload pointer) points to the start address of payload

// tested
#define CURR_ALLOC                                  (1 << 0)
#define PREV_ALLOC                                  (1 << 1)
#define SET_CURR_ALLOC_BIT(p)                       (WRITE_WORD(p, READ_WORD(p) | CURR_ALLOC))
#define CLR_CURR_ALLOC_BIT(p)                       (WRITE_WORD(p, READ_WORD(p) & ~CURR_ALLOC))
#define SET_PREV_ALLOC_BIT(p)                       (WRITE_WORD(p, READ_WORD(p) | PREV_ALLOC))
#define CLR_PREV_ALLOC_BIT(p)                       (WRITE_WORD(p, READ_WORD(p) & ~PREV_ALLOC))
#define PACK(size, alloc)                           ((size) | (alloc))
#define SIZE_MASK                                   (~(ALIGNMENT - 1))
#define SET_SIZE(hdrp, size)                        (WRITE_WORD(hdrp, (READ_WORD(hdrp) & (ALIGNMENT-1)) | size))
#define GET_SIZE(hdrp)                              (READ_WORD(hdrp) & SIZE_MASK)  // get block size from hdrp
#define GET_ALLOC(hdrp)                             (READ_WORD(hdrp) & 0x1)  // get alloc bit from hdrp
#define GET_PREV_ALLOC(hdrp)                        (READ_WORD(hdrp) & 0x2)  // get alloc bit of previous block
#define HDRP_USE_PLDP(pldp)                         ((char*)(pldp) - WSIZE)  // get hdrp using pldp
#define FTRP_USE_PLDP(pldp)                         ((char*)(pldp) + GET_SIZE(HDRP_USE_PLDP(pldp)) - DSIZE)  // get ftrp using pldp

// not yet
/*
#define PREV_FTRP_USE_PLDP(pldp)                    ((char*)HDRP_USE_PLDP(pldp) - WSIZE)  // get previous block footer using current pldp
#define NEXT_HDRP_USE_PLDP(pldp)                    (HDRP_USE_PLDP((char*)(pldp) + GET_SIZE(HDRP_USE_PLDP(pldp))))  // get the next block header using current pldp
#define PREV_HDRP_USE_PLDP(pldp)                    (HDRP_USE_PLDP((char*)(pldp) - GET_SIZE(PREV_FTRP_USE_PLDP(pldp))))) // get the header of previous using current pldp
#define NEXT_PLDP_USE_PLDP(pldp)                    ((char*)(pldp) + GET_SIZE(HDRP_USE_PLDP(pldp)))  // get next block pldp using current pldp
#define PREV_PLDP_USE_PLDP(pldp)                    ((char*)(pldp) - GET_SIZE(PREV_FTRP_USE_PLDP(pldp)))  // get previous block pldp using current pldp
#define NEXT_PLDP_USE_HDRP(hdrp)                    ((char*)(hdrp) + GET_SIZE(hdrp) + WSIZE)  // get the next pldp using current hdrp
#define NEXT_HDRP_USE_HDRP(hdrp)                    ((char*)(hdrp) + GET_SIZE(hdrp))
*/

// For Debug
#define __HEAP_CHECK__
#ifdef __HEAP_CHECK__
static void* heap_head = NULL; // points to heap start
static void* heap_tail = NULL; // points to heap end, as Epilogue header
typedef struct HeapStruct {
  char *bk_head;  // block head
  char *bk_tail;  // block tail
  char *pl_head;  // payload head
  char *pl_tail;  // payload tail
  size_t bk_size; // block size
  size_t pl_size; // payload size;
  int index;
  struct HeapStruct *next;
} HeapStruct;

static HeapStruct *alloc_list;
static HeapStruct *free_list;

static void add_to_alloc_list(const void *ptr, size_t pl_size, size_t bk_size);
static void delete_from_alloc_list(const void *ptr);
static void add_to_free_list(const void *ptr);
static HeapStruct* delete_from_list(HeapStruct *list, const void *ptr);
static HeapStruct* search_list(const HeapStruct *list, const void *ptr);
static void save_index(void);
static void check_index(void);
static int check_heap(void);
void show_heap(void);

#endif

static void* heap_listp = NULL;

// Function Declaration
void pointer_macro_test(void);
static void *extend_heap(size_t size);
static void *coalesce(void *pldp);
// the input size should be aligned
static void *find_first_fit(size_t asize);
// the input size should be aligned
static void place_and_split(void *pldp, size_t asize);

static void *forward_collect(void *hdrp, size_t *collected_size, size_t target_size);
static void *backward_collect(void *hdrp, size_t target_size);
void mm_memcpy(void *dst, void *src, size_t num);

static void stack_trace(void);


/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
  // Why 4 * WSIZE
  // 1 WSIZE for 0 padding
  // 2 WSIZE for Prologue block
  // 1 WSIZE for Epllogue block
  if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void*)(-1)) {
    return -1;
  }

  #ifdef __HEAP_CHECK__
  heap_head = heap_listp;
  heap_tail = heap_listp + 3 * WSIZE;
  #endif

  WRITE_WORD(heap_listp, 0); // 1 WSIZE for 0 padding
  WRITE_WORD(heap_listp + WSIZE, PACK(DSIZE, PREV_ALLOC | CURR_ALLOC));  // Prologue header
  WRITE_WORD(heap_listp + 2 * WSIZE, PACK(DSIZE, PREV_ALLOC | CURR_ALLOC)); // Prologue footer
  WRITE_WORD(heap_listp + 3 * WSIZE, PACK(0, PREV_ALLOC | CURR_ALLOC)); // Epilogue header
  heap_listp += 3 * WSIZE;


  void* pldp = extend_heap(CHUNKSIZE);
  if (pldp == NULL) return -1;
  return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
  // Ignore spurious requests
  if (!size) return NULL;
  void *ret = NULL;
  size_t asize = ALIGN(size + WSIZE);
  // DebugStr("mm_malloc with size: %u\n", asize);
  void *pldp = find_first_fit(asize);
  if (pldp) {
    place_and_split(pldp, asize);
    ret = pldp;
  } else if ((pldp = extend_heap(asize))) {
    place_and_split(pldp, asize);
    ret = pldp;
  }

#ifdef __HEAP_CHECK__
  DebugStr("\ncalled mm_malloc with size: %u, pointer = %x\n", size, ret);
  add_to_alloc_list(ret, size, asize);
#endif

  return ret;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
#ifdef __HEAP_CHECK__
  DebugStr("\ncalled mm_free with %x\n", ptr);
#endif
  assert(search_list(alloc_list, ptr));
  void *hdrp = (char*)ptr - WSIZE;
  size_t size = GET_SIZE(hdrp);
  CLR_CURR_ALLOC_BIT(hdrp);
  WRITE_WORD((char*)hdrp + size - WSIZE, READ_WORD(hdrp)); // footer is same as header
  CLR_PREV_ALLOC_BIT((char*)hdrp + size); // clear the prev_alloc bit of next block, since current block is free
#ifdef __HEAP_CHECK__
  delete_from_alloc_list(ptr);
#endif
  coalesce(ptr); // immediate coalescing
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
//
void *mm_realloc(void *ptr, size_t size) {
  if (!ptr) return mm_malloc(size);

#ifdef __HEAP_CHECK__
  save_index();
#endif

  void *hdrp = HDRP_USE_PLDP(ptr);
  size_t old_size = GET_SIZE(hdrp);
  size_t ori_size = old_size; // original block size
  size_t target_size = ALIGN(size + WSIZE); // realloc target size

#ifdef __HEAP_CHECK__
  DebugStr("\n\nCall mm_realloc: with old_size:%d, target_size:%d, ptr = %x\n", old_size, target_size, ptr);
  assert(search_list(alloc_list, ptr));
#endif

  backward_collect(hdrp, target_size);
  assert(hdrp == (char*)ptr - WSIZE);
  old_size = GET_SIZE(hdrp); // update the block size pointed by hdrp
  if (target_size <= old_size) {
    DebugStr("backward_collect...\n", old_size);
    place_and_split(ptr, target_size);
#ifdef __HEAP_CHECK__
    check_index();
    delete_from_alloc_list(ptr);
    add_to_alloc_list(ptr, size, GET_SIZE(HDRP_USE_PLDP(ptr)));
#endif
    return ptr;
  }

  DebugStr("before forward_collect...\n");
  size_t collected_size = 0;
  void *new_hdrp = forward_collect(hdrp, &collected_size, target_size);
  if (target_size <= collected_size + old_size) {
    size_t new_size = collected_size + old_size;
    mm_memcpy((char*)new_hdrp + WSIZE, ptr, ori_size - WSIZE); // copy payload
    size_t left_size = new_size - target_size;
    if (left_size && IS_ALIGN_WITH_MIN_BK_SIZE(left_size)) {
      void *split_hdrp = new_hdrp + target_size;
      WRITE_WORD(split_hdrp, PACK(left_size, PREV_ALLOC));
      WRITE_WORD((char*)split_hdrp + left_size - WSIZE, READ_WORD(split_hdrp));
      CLR_PREV_ALLOC_BIT((char*)split_hdrp + left_size);
      new_size = target_size;
      DebugStr("Split...");
    }
    SET_SIZE(new_hdrp, new_size);
    SET_CURR_ALLOC_BIT(new_hdrp);
    SET_PREV_ALLOC_BIT((char*)new_hdrp + new_size);
    DebugStr("forward_collect...\n");
#ifdef __HEAP_CHECK__
    check_index();
    delete_from_alloc_list(ptr);
    add_to_alloc_list((char*)new_hdrp + WSIZE, size, GET_SIZE(new_hdrp));
#endif
    return new_hdrp + WSIZE;
  }

  void *new_pldp = find_first_fit(target_size);
  if (new_pldp) {
    place_and_split(new_pldp, target_size);
    mm_memcpy(new_pldp, ptr, ori_size - WSIZE);
    mm_free(ptr);
    DebugStr("find_first_fit...\n");
#ifdef __HEAP_CHECK__
    check_index();
    add_to_alloc_list((char*)new_pldp, size, GET_SIZE(HDRP_USE_PLDP(new_pldp)));
#endif
    return new_pldp;
  }

  DebugStr("before extend_heap...\n");
  new_pldp = extend_heap(target_size);
  if (!new_pldp) return NULL;

  place_and_split(new_pldp, target_size);
  mm_memcpy(new_pldp, ptr, ori_size - WSIZE);
  mm_free(ptr);
  DebugStr("extend_heap...\n");
#ifdef __HEAP_CHECK__
  check_index();
#endif
  return new_pldp;
}

static void *extend_heap(size_t size) {
  char *hdrp;
  size_t asize = ALIGN_CHUNKSIZE(size);
  if ((void*)(hdrp = mem_sbrk(asize)) == (void*)(-1)) return NULL;
  hdrp = (char*)hdrp - WSIZE; // points to the old Epliogue header

  #ifdef __HEAP_CHECK__
  heap_tail += asize;
  #endif

  WRITE_WORD(hdrp, PACK(asize, GET_PREV_ALLOC(hdrp))); // set new header
  WRITE_WORD((char*)hdrp + asize - WSIZE, READ_WORD(hdrp)); // footer is same as header
  WRITE_WORD(hdrp + asize, PACK(0, CURR_ALLOC)); // write new epilogue block
  return coalesce((char*)hdrp + WSIZE);
}

static void *coalesce(void *pldp) {
  void *hdrp = HDRP_USE_PLDP(pldp);
  size_t size = GET_SIZE(hdrp);
  size_t prev_alloc = GET_PREV_ALLOC(hdrp); // read current header to get the privous block allocate bit
  size_t next_alloc = GET_ALLOC((char*)hdrp + size); // read next block header to get its allocate bit

  // previous and next are both allocated
  if (prev_alloc && next_alloc) {
    return pldp;
  }

  // previous is free but next is allocated
  if (!prev_alloc && next_alloc) {
    void *prev_hdrp = (char*)hdrp - GET_SIZE((char*)hdrp - WSIZE); // get previous block header pointer
    WRITE_WORD(prev_hdrp, READ_WORD(prev_hdrp) + size); // write new header in previous block
    WRITE_WORD((char*)hdrp + size - WSIZE, READ_WORD(prev_hdrp)); // write new footer in current block
    return (char*)prev_hdrp + WSIZE;
  }

  // next is free but previous is allocated
  if (prev_alloc && !next_alloc) {
    void *next_hdrp = (char*)hdrp + size;
    size_t next_size = GET_SIZE(next_hdrp);
    WRITE_WORD(hdrp, READ_WORD(hdrp) + next_size); // write new header in current block
    WRITE_WORD((char*)next_hdrp + next_size - WSIZE, READ_WORD(hdrp)); // write new footer in next block
    return pldp;
  }

  // previous and next are both free
  size_t prev_size = GET_SIZE((char*)hdrp - WSIZE);
  void *next_hdrp = (char*)hdrp + size;
  void *prev_hdrp = (char*)hdrp - prev_size;
  size_t next_size = GET_SIZE(next_hdrp);
  WRITE_WORD(prev_hdrp, READ_WORD(prev_hdrp) + size + next_size); // write new header
  WRITE_WORD((char*)next_hdrp + next_size - WSIZE, READ_WORD(prev_hdrp));
  return (char*)prev_hdrp + WSIZE;
}

// linear search
static void *find_first_fit(size_t asize) {
  void *hdrp = heap_listp;
  size_t b_size;
  do {
    b_size = GET_SIZE(hdrp);
    if (asize <= b_size && !GET_ALLOC(hdrp)) {
      return (char*)hdrp + WSIZE;
    }
    hdrp = (char*)hdrp + b_size;
  } while (b_size > 0);
  return NULL;
}

static void place_and_split(void *pldp, size_t asize) {
  void *hdrp = HDRP_USE_PLDP(pldp);
  size_t b_size = GET_SIZE(hdrp);
  size_t left_size = b_size - asize;
  // split
  if (left_size && IS_ALIGN_WITH_MIN_BK_SIZE(left_size)) { // reminder > 0 and is multiple of minimum blockasize
    void *new_free_hdrp = (char*)hdrp + asize;
    WRITE_WORD(new_free_hdrp, PACK(left_size, 0));
    WRITE_WORD((char*)new_free_hdrp + left_size - WSIZE, READ_WORD(new_free_hdrp));
    b_size = asize;
  }
  SET_SIZE(hdrp, b_size);
  SET_CURR_ALLOC_BIT(hdrp);
  void *next_hdrp = (char*)hdrp + b_size;
  SET_PREV_ALLOC_BIT(next_hdrp);
  #ifdef __HEAP_CHECK__
  SET_PREV_ALLOC_BIT((char*)next_hdrp + GET_SIZE(next_hdrp) - WSIZE);
  #endif __HEAP_CHECK__
}

static void *forward_collect(void *hdrp, size_t *collected_size, size_t target_size) {
  if (GET_PREV_ALLOC(hdrp)) return hdrp;
  size_t old_size = GET_SIZE(hdrp);
  *collected_size = GET_SIZE((char*)hdrp - WSIZE);
  void *ret = (char*)hdrp - *collected_size;
  void *ftrp = (char*)hdrp - WSIZE;
  while (*collected_size + old_size < target_size && !GET_PREV_ALLOC(ret)) {
    size_t next_size = GET_SIZE((char*)ret - WSIZE);
    ret = (char*)ret - next_size;
    WRITE_WORD(ret, READ_WORD(ret) + *collected_size);
    *collected_size = GET_SIZE(ret);
    WRITE_WORD(ftrp, READ_WORD(ret));
  }
  return ret;
}

static void *backward_collect(void *hdrp, size_t target_size) {
  DebugStr("inside backward_collect...\n");
  void *next_hdrp = (char*)hdrp + GET_SIZE(hdrp);
  while (GET_SIZE(hdrp) < target_size && !GET_ALLOC(next_hdrp)) {
    size_t next_size = GET_SIZE(next_hdrp);
    WRITE_WORD(hdrp, READ_WORD(hdrp) + next_size); // update header
    next_hdrp = (char*)next_hdrp + next_size;
    // WRITE_WORD((char*)next_hdrp - WSIZE, READ_WORD(hdrp));
    SET_PREV_ALLOC_BIT(next_hdrp);
    // CLR_PREV_ALLOC_BIT(next_hdrp);
  }
}

void mm_memcpy(void *dst, void *src, size_t num) {
  char* ddst = (char*)dst;
  char* ssrc = (char*)src;
  size_t i = 0;
  for (i = 0; i < num; ++i) {
    *ddst++ = *ssrc++;
  }
}


void pointer_macro_test() {
  assert(CURR_ALLOC == 0x1);
  assert(PREV_ALLOC == 0x2);
  assert(SIZE_MASK == 0xFFFFFFF8);

  size_t word1 = 0xF0E1B1;
  size_t word2;
  assert(READ_WORD(&word1) == word1);
  WRITE_WORD(&word2, word1);
  assert(READ_WORD(&word2) == word1);

  size_t header1 = 0x0;
  SET_CURR_ALLOC_BIT(&header1);
  assert(header1 == 0x1);
  CLR_CURR_ALLOC_BIT(&header1);
  assert(header1 == 0x0);

  SET_PREV_ALLOC_BIT(&header1);
  assert(header1 == 0x2);
  CLR_PREV_ALLOC_BIT(&header1);
  assert(header1 == 0x0);

  header1 = 0x3;
  SET_SIZE(&header1, 0x300);
  assert(header1 == 0x303);
  assert(GET_SIZE(&header1) == 0x300);
  assert(GET_ALLOC(&header1) == 0x1);
  assert(GET_PREV_ALLOC(&header1) == 0x2);

  assert(HDRP_USE_PLDP((void*)&header1 + WSIZE) == (void*)&header1);
  assert(FTRP_USE_PLDP((void*)&header1 + WSIZE) == (void*)((char*)&header1 + 0x300 - WSIZE));
}

void ToBinaryStr(size_t num, int sep) {
  int size = sizeof(num) * 8;
  int i;
  for (i = size - 1; i >= 0; --i) {
    fprintf(stderr, "%d", !!(num & (1 << i)));
    if (sep && !(i % 4)) {
      fprintf(stderr, " ");
    }
  }
  fprintf(stderr, "\n");
}

void ToHexStr(size_t num, int sep) {
  static size_t mask = (1 << 4) - 1;
  int size = sizeof(num) * 8;
  int i;
  for (i = size - 4; i >= 0; i -= 4) {
    fprintf(stderr, "%x", (num & (mask << i)) >> i);
    if (sep && !(i % 8)) {
      fprintf(stderr, " ");
    }
  }
  fprintf(stderr, "\n");
}

#ifdef __HEAP_CHECK__
static void add_to_alloc_list(const void *ptr, size_t pl_size, size_t bk_size) {
  if (ptr == NULL) return;
  assert(search_list(alloc_list, ptr) == NULL);
  DebugStr("Add %x\n", ptr);
  HeapStruct* new_node = (HeapStruct*)malloc(sizeof(HeapStruct));
  new_node->bk_head = HDRP_USE_PLDP(ptr);
  new_node->bk_tail = (char*)HDRP_USE_PLDP(ptr) + bk_size;
  new_node->pl_head = ptr;
  new_node->pl_tail = (char*)pl_size;
  new_node->bk_size = bk_size;
  new_node->pl_size = pl_size;
  new_node->next = alloc_list; // insert it in the front of the list
  new_node->index = -1;
  alloc_list = new_node;
}

static void delete_from_alloc_list(const void *ptr) {
  assert(ptr);
  DebugStr("delete %x\n", ptr);
  HeapStruct *node = delete_from_list(alloc_list, ptr);
  assert(node);
  free(node);
}

static void add_to_free_list(const void *ptr) {
  if (ptr == NULL) return;
  HeapStruct *node = delete_from_list(alloc_list, ptr);
  assert(node);
  free(node);
}

static HeapStruct* delete_from_list(HeapStruct *list, const void *ptr) {
  assert(list);
  HeapStruct *ret = NULL;
  if (list->pl_head == ptr) {
    ret = list;
    alloc_list = list->next;
    ret->next = NULL;
    return ret;
  }

  while (list->next && list->next->pl_head != ptr) {
    list = list->next;
  }

  if (list->next) {
    ret = list->next;
    list->next = ret->next;
    ret->next = NULL;
  }

  return ret;
}

static HeapStruct* search_list(const HeapStruct *list, const void *ptr) {
  assert(ptr);
  while (list && list->pl_head != ptr) {
    list = list->next;
  }
  return list;
}

static void save_index(void) {
  HeapStruct* alist = alloc_list;
  while (alist) {
    alist->index = *((char*)alist->pl_head);
    alist = alist->next;
  }
}

static void check_index(void) {
  HeapStruct* alist = alloc_list;
  while (alist) {
    assert(alist->index != -1);
    int i = 0;
    for (i = 0; i < alist->pl_size; ++i) {
      assert(*((char*)alist->pl_head + i) == alist->index);
    }
    alist = alist->next;
  }
}

static int check_heap() {
  ;
}

void show_heap(void) {
  void* p = heap_head;
  DebugStr("-----------------\n");
  DebugStr("heap_head = %x, heap size = %u\n", p, (char*)heap_tail  - (char*)heap_head - 3 * WSIZE);
  DebugStr("-----------------\n");
  DebugStr("%x\n", (char*)p + WSIZE);
  DebugStr("%x\n", (char*)p + 2 * WSIZE);
  DebugStr("-----------------\n");
  p = (char*)p + 3 * WSIZE;
  assert(p <= heap_tail);
  while (GET_SIZE(p) > 0) {
    assert(p <= heap_tail);
    DebugStr("hdrp:%x val = ", p);
    ToHexStr(*(size_t*)p, 1);
    DebugStr("size = %u, ALLOC = %d, PREV_ALLOC = %d\n", GET_SIZE(p), !!GET_ALLOC(p), !!GET_PREV_ALLOC(p));
    void *ftrp = (char*)p + GET_SIZE(p) - WSIZE;
    DebugStr("ftrp:%x val = ", ftrp);
    ToHexStr(*(size_t*)ftrp, 1);
    if (!GET_ALLOC(p)) { // if current block is not allocated, header = footer
      assert(READ_WORD(p) == READ_WORD(ftrp));
    }
    DebugStr("-----------------\n");
    p += GET_SIZE(p);
  }
  DebugStr("heap_tail = %x\n", p);
  DebugStr("-----------------\n");
}

static void stack_trace(size_t level) {
  void *array[NUM_STACK_TRACE];
  size_t size;
  char **strings;
  size_t i;
  size = backtrace(array, NUM_STACK_TRACE);
  strings = backtrace_symbols(array, size);

  size = size < level ? size : level;
  for (i = 1; i < size; ++i) {
    fprintf(stderr, "%s\n", strings[i]);
  }
  free(strings);
}
#endif