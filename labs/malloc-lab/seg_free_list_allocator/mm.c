/*
This version used the segregated free list
Implementation Details
0. Possible Maximum allocate size: 32GB
1. Free Block Organization: use segregated free list
2. Coalescing: use immediate coalescing with boundary tags
3. Placement: first-fit
4. Splitting: Splitting only if the size of the reminder would equal or exceed the minimum block size
5. Heap Structure
   [ 1 word padding | block 0 | block 1 | ... ]
6. Block Structure:
   - allocated block: [1 word Header | ... payload ... | optional padding ]
   - free block: [ 1 word header] | 1 word prev_pointer | 1 word next_pointer | .... | 1 word footer ]
7. Minimum block size: 16 bytes
8. Header Structure:
   - [31 - 3] bit for size
   - [2] not used
   - [1] pre_alloc    : indicate whether or not previous block is allocated
   - [0] alloc        : indicate whether or not current block is allocated
9. Size classes: power of 2
   - [16 ~ 31]
   - [32 ~ 63]
   - ...
   - [2^31 ~ (2^32)-1]
   - total: 28 classes, so we need a static global void* array of size 28
10. How to order free block in each list ?
   - choice 1 ordering by address
   - choice 2 LIFO
11. Analysis
   - malloc is linear to the size of one size class, worse case O(N)
   - free is linear to the size of one size class, if choose ordering by address, otherwise constant time
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include "mm.h"
#include "memlib.h"

#define __LIFO_ORDERING__
// #define __USE_FIRST_FIT__
// #define __HEAP_CHECK__
// #define __LOG_TO_STDERR__

#ifdef __LOG_TO_STDERR__
#define DebugStr(args...)   fprintf(stderr, args);
#else
#define DebugStr(args...)
#endif

#ifdef __HEAP_CHECK__
#include <string.h>
#include <execinfo.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#endif

team_t team = {
    /* Team name */
    "CSAPP",
    /* First member's full name */
    "CSAPP",
    /* First member's email address */
    "CSAPP@CSAPP.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

#ifdef __HEAP_CHECK__
typedef void handler_t(int);

// record the allocated block only
typedef struct HeapStruct {
  char *bk_head;  // block head
  char *bk_tail;  // block tail
  char *pl_head;  // payload head
  char *pl_tail;  // payload tail, pl_tail is possible to equal to bk_tail
  size_t bk_size; // block size
  size_t pl_size; // payload size;
  int index;
  struct HeapStruct *next;
} HeapStruct;

static HeapStruct *alloc_list = NULL;
static HeapStruct *free_list = NULL;

static int writable = 1;

void add_to_alloc_list(const void *ptr, const size_t pl_size, const size_t bk_size);
void delete_from_alloc_list(const void *ptr);
HeapStruct* delete_from_list(HeapStruct *list, const void *ptr);
HeapStruct* search_list(const HeapStruct *list, const void *ptr);
int addr_is_allocated(const char *addr);
int addr_is_payload(const char *addr);
int within_heap(const void *addr);
void show_heap(void);
void show_alloc_list(void);
handler_t *Signal(int signum, handler_t *handler);
void print_stack_trace(int signum);
void to_hex_str(size_t num, int sep);
void to_binary_str(size_t num, int sep);
size_t heap_size(void);
int segregated_free_list_valid(void);
#endif

static void* heap_head = NULL; // points to heap start
static void* heap_tail = NULL; // points to heap end, one byte after Epilogue header

#include "mm_macros.h"

static void* heap_listp = NULL;
size_t index_array[] = {
  1 << 4, 1 << 5, 1 << 6, 1 << 7, 1 << 8, 1 << 9, 1 << 10,
  1 << 11, 1 << 12, 1 << 13, 1 << 14, 1 << 15, 1 << 16, 1 << 17,
  1 << 18, 1 << 19, 1 << 20, 1 << 21, 1 << 22, 1 << 23, 1 << 24,
  1 << 25, 1 << 26, 1 << 27, 1 << 28, 1 << 29, 1 << 30, 1 << 31
};

void *segregated_free_list[sizeof(index_array) / sizeof(index_array[0])];
int index_arr_size = sizeof(index_array) / sizeof(index_array[0]);

void *extend_heap(size_t size);
void *realloc_extend_heap(size_t size);
void *coalesce(void *hdrp);
void *find_fit(size_t asize);
void *place_and_split(void *hdrp, size_t asize);
void *realloc_place_and_split(void *hdrp, size_t asize);

void *forward_collect(void *hdrp, size_t *collected_size, size_t target_size);
void *backward_collect(void *hdrp, size_t target_size);
void mm_memcpy(void *dst, void *src, size_t num);

// helper functions
int find_index(size_t size);
void init_free_block(void *hdrp, size_t bk_size);
void insert_into_size_class(void *hdrp, int index);
void delete_from_size_class(void *hdrp, int index);
int is_in_size_class(void *hdrp, int index);
void print_list(void *hdrp);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {

#ifdef __HEAP_CHECK__
  Signal(SIGABRT, print_stack_trace);
  heap_head = NULL;
  heap_tail = NULL;
  alloc_list = NULL;
#endif
  // 1 WSIZE for heap-start padding
  // 1 WSIZE for heap-end padding
  if ((heap_listp = mem_sbrk(2 * WSIZE)) == (void*)(-1)) {
    return -1;
  }

  heap_head = heap_listp;
  heap_tail = heap_listp + 2 * WSIZE;

  WRITE_WORD(heap_listp, PACK(0, CURR_ALLOC));  // heap-start 0 padding
  WRITE_WORD(heap_listp + WSIZE, PACK(0, PREV_ALLOC | CURR_ALLOC));  // heap-end 0 padding
  int i = 0;
  for (i = 0; i < index_arr_size; ++i) {
    segregated_free_list[i] = NULL;
  }

#ifdef __HEAP_CHECK__
  assert(heap_size() == 2 * WSIZE);
#endif
  return extend_heap(CHUNKSIZE) == NULL ? -1 : 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
  // Ignore spurious requests
  if (!size) return NULL;

  void *ret = NULL;
  size_t asize = ALIGN_WITH_MIN_BK_SIZE(size + WSIZE);
  void *head = find_fit(asize);
  if (head) {

#ifdef __HEAP_CHECK__
    assert(within_heap(head));
    assert(!addr_is_allocated(head));
    assert(!addr_is_payload(head));
#endif

    head = place_and_split(head, asize);
    ret = (char*)head + WSIZE;
  } else {
    void *heap_end_padding = (char*)heap_tail - WSIZE;
    if (!GET_PREV_ALLOC(heap_end_padding)) {
      void *footer = (char*)heap_end_padding - WSIZE;
      size_t end_bk_size = GET_SIZE(footer);
      head = extend_heap(asize - end_bk_size);
    } else {
      head = extend_heap(asize);
    }
    head = place_and_split(head, asize);
    ret = (char*)head + WSIZE;
  }
  // } else if ((head = extend_heap(asize))) {
  //   place_and_split(head, asize);
  //   ret = (char*)head + WSIZE;
  // }

  // } else {
  //   if (!GET_PREV_ALLOC((char*)heap_tail - WSIZE)) {
  //     DebugStr("prev_alloc is 0\n");
  //     head = extend_heap(asize - GET_SIZE((char*)heap_tail - DSIZE));
  //   }
  //   place_and_split(head, asize);
  //   return (char*)head + WSIZE;
  // }

#ifdef __HEAP_CHECK__
  assert(ret);
  add_to_alloc_list(ret, size, asize);
#endif

  return ret;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
#ifdef __HEAP_CHECK__
  assert(ptr);
  delete_from_alloc_list(ptr);
#endif
  void *head = (char*)ptr - WSIZE;
  size_t size = GET_SIZE(head);
  init_free_block(head, size);
  insert_into_size_class(head, find_index(size));
  coalesce(head);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
  if (!ptr) return mm_malloc(size);

  void *hdrp = HDRP_USE_PLDP(ptr);
  size_t old_size = GET_SIZE(hdrp);
  size_t ori_size = old_size;
  size_t target_size = ALIGN_WITH_MIN_BK_SIZE(size + WSIZE);
  backward_collect(hdrp, target_size);
  old_size = GET_SIZE(hdrp);
  if (target_size <= old_size) {

#ifdef __HEAP_CHECK__
    delete_from_alloc_list(ptr);
#endif

    hdrp = realloc_place_and_split(hdrp, target_size);

#ifdef __HEAP_CHECK__
    add_to_alloc_list(ptr, size, target_size);
#endif

    return (char*)hdrp + WSIZE;
  }

  /*
  size_t collected_size = 0;
  void *new_hdrp = forward_collect(hdrp, &collected_size, target_size);
  if (target_size <= collected_size + old_size) {
#ifdef __HEAP_CHECK__
    delete_from_alloc_list(ptr);
#endif

    size_t new_size = collected_size + old_size;
    mm_memcpy((char*)new_hdrp + WSIZE, ptr, ori_size - WSIZE);
    size_t left_size = new_size = target_size;
    if (left_size && IS_ALIGN_WITH_MIN_BK_SIZE(left_size)) {
      void *split_hdrp = (char*)new_hdrp + target_size;
      WRITE_WORD(split_hdrp, PACK(left_size, PREV_ALLOC));
      WRITE_WORD((char*)split_hdrp + left_size - WSIZE, READ_WORD(split_hdrp));
      CLR_PREV_ALLOC_BIT((char*)split_hdrp + left_size);
      new_size = target_size;
      insert_into_size_class(split_hdrp, find_index(left_size));
    }

    SET_SIZE(new_hdrp, new_size);
    SET_CURR_ALLOC_BIT(new_hdrp);
    SET_PREV_ALLOC_BIT((char*)new_hdrp + new_size);
#ifdef __HEAP_CHECK__
    add_to_alloc_list(new_hdrp + WSIZE, size, target_size);
#endif
    return (char*)new_hdrp + WSIZE;
  }*/

  void *new_hdrp = find_fit(target_size);
  if (new_hdrp) {
    new_hdrp = place_and_split(new_hdrp, target_size);
    mm_memcpy((char*)new_hdrp + WSIZE, ptr, ori_size - WSIZE);
    mm_free(ptr);
#ifdef __HEAP_CHECK__
    add_to_alloc_list((char*)new_hdrp + WSIZE, size, target_size);
#endif
    return (char*)new_hdrp + WSIZE;
  }

  // new_hdrp = extend_heap(target_size);
  new_hdrp = realloc_extend_heap(target_size);
  if (!new_hdrp) return NULL;
  new_hdrp = place_and_split(new_hdrp, target_size);
  mm_memcpy((char*)new_hdrp + WSIZE, ptr, ori_size - WSIZE);
  mm_free(ptr);
#ifdef __HEAP_CHECK__
  add_to_alloc_list((char*)new_hdrp + WSIZE, size, target_size);
#endif
  return (char*)new_hdrp + WSIZE;
}

int find_index(size_t size) {
  int low = 0, high = index_arr_size;
  int ret = -1;
  int mid;
  while (low < high) {
    mid = (high + low) >> 1;
    if (size < index_array[mid]) {
      high = mid;
    } else {
      low = mid + 1;
    }
  }
  return low - 1;
}

void *extend_heap(size_t size) {
  char *hdrp = NULL;
  size_t asize = ALIGN_CHUNKSIZE(size);
  if ((void*)(hdrp = mem_sbrk(asize)) == (void*)(-1)) return NULL;

#ifdef __HEAP_CHECK__
  assert(hdrp == heap_tail);
  assert(!addr_is_allocated(hdrp));
  assert(!addr_is_allocated((char*)hdrp + asize));
#endif

  heap_tail += asize;

  hdrp = (char*)hdrp - WSIZE; // points to heap-end 0 padding
  init_free_block(hdrp, asize); // pack header and footer
  insert_into_size_class(hdrp, find_index(asize));

#ifdef __HEAP_CHECK__
  assert((hdrp + asize) == ((char*)heap_tail - WSIZE));
#endif

  WRITE_WORD((char*)hdrp + asize, PACK(0, 1)); // write new heap-end padding
  return coalesce((char*)hdrp);
}

void *realloc_extend_heap(size_t size) {
  char *hdrp = NULL;
  size_t asize = ALIGN_RECHUNKSIZE(size);
  if ((void*)(hdrp = mem_sbrk(asize)) == (void*)(-1)) return NULL;

#ifdef __HEAP_CHECK__
  assert(hdrp == heap_tail);
  assert(!addr_is_allocated(hdrp));
  assert(!addr_is_allocated((char*)hdrp + asize));
#endif

  heap_tail += asize;

  hdrp = (char*)hdrp - WSIZE; // points to heap-end 0 padding
  init_free_block(hdrp, asize); // pack header and footer
  insert_into_size_class(hdrp, find_index(asize));

#ifdef __HEAP_CHECK__
  assert((hdrp + asize) == ((char*)heap_tail - WSIZE));
#endif

  WRITE_WORD((char*)hdrp + asize, PACK(0, 1)); // write new heap-end padding
  return coalesce((char*)hdrp);
}

void *coalesce(void *hdrp) {
  size_t size = GET_SIZE(hdrp);
  size_t prev_alloc = GET_PREV_ALLOC(hdrp); // read current header to get the privous block allocate bit
  size_t next_alloc = GET_ALLOC((char*)hdrp + size); // read next block header to get its allocate bit
  // previous and next are both allocated
  if (prev_alloc && next_alloc) {
    return hdrp;
  }

  // previous is free but next is allocated
  if (!prev_alloc && next_alloc) {
    size_t prev_bk_size = GET_SIZE((char*)hdrp - WSIZE);
    void *prev_hdrp = (char*)hdrp - prev_bk_size;
    delete_from_size_class(prev_hdrp, find_index(prev_bk_size)); // delete previous block from free list
    delete_from_size_class(hdrp, find_index(size)); // delete current block from free list
    size_t new_size = prev_bk_size + size;
    init_free_block(prev_hdrp, new_size);
    insert_into_size_class(prev_hdrp, find_index(new_size));
    return (char*)prev_hdrp;
  }

  // next is free but previous is allocated
  if (prev_alloc && !next_alloc) {
    void *next_hdrp = (char*)hdrp + size;
    size_t next_bk_size = GET_SIZE(next_hdrp);
    delete_from_size_class(next_hdrp, find_index(next_bk_size));
    delete_from_size_class(hdrp, find_index(size));

    size_t new_size = next_bk_size + size;
    init_free_block(hdrp, new_size);
    insert_into_size_class(hdrp, find_index(new_size));
    return hdrp;
  }

  // previous and next are both free
  size_t prev_bk_size = GET_SIZE((char*)hdrp - WSIZE);
  void *prev_hdrp = (char*)hdrp - prev_bk_size;
  void *next_hdrp = (char*)hdrp + size;
  size_t next_bk_size = GET_SIZE(next_hdrp);

  delete_from_size_class(prev_hdrp, find_index(prev_bk_size));
  delete_from_size_class(next_hdrp, find_index(next_bk_size));
  delete_from_size_class(hdrp, find_index(size));
  size_t new_size = prev_bk_size + next_bk_size + size;
  init_free_block(prev_hdrp, new_size);
  insert_into_size_class(prev_hdrp, find_index(new_size));
  return (char*)prev_hdrp;
}

// linear search
void *find_fit(size_t asize) {
#ifdef __USE_FIRST_FIT__
  int index = find_index(asize);
  while (index < index_arr_size) {
    void *head = segregated_free_list[index];
    while (head) {
      if (GET_SIZE(head) >= asize) {
        return head;
      }
      head = GET_NEXT_PTR(head);
    }
    ++index;
  }
#else
  int index = find_index(asize);
  void *ret = NULL;
  size_t min_size = (1 << 31) - 1;
  while (index < index_arr_size) {
    void *head = segregated_free_list[index];
    while (head) {
      size_t h_size = GET_SIZE(head);
      if (h_size >= asize && h_size < min_size) {
        ret = head;
        min_size = h_size;
      }
      head = GET_NEXT_PTR(head);
    }
    if (ret == NULL) {
      ++index;
    } else {
      return ret;
    }
  }
#endif
  return NULL;
}

void *place_and_split(void *hdrp, size_t asize) {
  size_t bk_size = GET_SIZE(hdrp);
#ifdef __HEAP_CHECK__
  assert(bk_size >= asize);
#endif
  delete_from_size_class(hdrp, find_index(bk_size));
  size_t left_size = bk_size - asize;
  if (left_size >= bk_size) {
    if (left_size && IS_ALIGN_WITH_MIN_BK_SIZE(left_size)) {
      SET_SIZE(hdrp, asize);
      SET_CURR_ALLOC_BIT(hdrp);
      void *new_free_hdrp = (char*)hdrp + asize;
      WRITE_WORD(new_free_hdrp, PREV_ALLOC);
      init_free_block(new_free_hdrp, left_size);
      insert_into_size_class(new_free_hdrp, find_index(left_size));
    } else {
      SET_CURR_ALLOC_BIT(hdrp);
      SET_PREV_ALLOC_BIT((char*)hdrp + bk_size);
    }
    return hdrp;
  } else {
    if (left_size && IS_ALIGN_WITH_MIN_BK_SIZE(left_size)) {
      SET_SIZE(hdrp, left_size);
      init_free_block(hdrp, left_size);
      insert_into_size_class(hdrp, find_index(left_size));

      void *new_free_hdrp = (char*)hdrp + left_size;
      WRITE_WORD(new_free_hdrp, PACK(asize, 1));
      SET_PREV_ALLOC_BIT((char*)new_free_hdrp + asize);
      return new_free_hdrp;
    } else {
      SET_CURR_ALLOC_BIT(hdrp);
      SET_PREV_ALLOC_BIT((char*)hdrp + bk_size);
      return hdrp;
    }
  }
}

void *realloc_place_and_split(void *hdrp, size_t asize) {
  size_t bk_size = GET_SIZE(hdrp);
#ifdef __HEAP_CHECK__
  assert(bk_size >= asize);
#endif
  size_t left_size = bk_size - asize;
  if (left_size && IS_ALIGN_WITH_MIN_BK_SIZE(left_size)) {
    SET_SIZE(hdrp, asize);
    SET_CURR_ALLOC_BIT(hdrp);
    void *new_free_hdrp = (char*)hdrp + asize;
    WRITE_WORD(new_free_hdrp, PREV_ALLOC);
    init_free_block(new_free_hdrp, left_size);
    insert_into_size_class(new_free_hdrp, find_index(left_size));
  } else {
    SET_CURR_ALLOC_BIT(hdrp);
    SET_PREV_ALLOC_BIT((char*)hdrp + bk_size);
  }
  return hdrp;
}

void *forward_collect(void *hdrp, size_t *collected_size, size_t target_size) {
  if (GET_PREV_ALLOC(hdrp)) return hdrp;
  size_t old_size = GET_SIZE(hdrp);
  *collected_size = GET_SIZE((char*)hdrp - WSIZE);
  void *ret = (char*)hdrp - *collected_size;
  void *ftrp = (char*)hdrp - WSIZE;

  delete_from_size_class(ret, find_index(GET_SIZE(ret)));

  while (*collected_size + old_size < target_size && !GET_PREV_ALLOC(ret)) {
    size_t next_size = GET_SIZE((char*)ret - WSIZE);
    ret = (char*)ret - next_size;
    delete_from_size_class(ret, find_index(next_size));

    WRITE_WORD(ret, READ_WORD(ret) + *collected_size);
    *collected_size = GET_SIZE(ret);
    WRITE_WORD(ftrp, READ_WORD(ret));
  }
  return ret;
}

void *backward_collect(void *hdrp, size_t target_size) {
  void *next_hdrp = (char*)hdrp + GET_SIZE(hdrp);
  while (GET_SIZE(hdrp) < target_size && !GET_ALLOC(next_hdrp)) {
    size_t next_size = GET_SIZE(next_hdrp);
    delete_from_size_class(next_hdrp, find_index(next_size));
    WRITE_WORD(hdrp, READ_WORD(hdrp) + next_size); // update header
    next_hdrp = (char*)next_hdrp + next_size;
    SET_PREV_ALLOC_BIT(next_hdrp);
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

void init_free_block(void *hdrp, size_t bk_size) {
  WRITE_WORD(hdrp, PACK(bk_size, GET_PREV_ALLOC(hdrp))); // write new header and keep prev_alloc bit
  WRITE_WORD((char*)hdrp + bk_size - WSIZE, READ_WORD(hdrp)); // write footer
  SET_PREV_PTR(hdrp, NULL);
  SET_NEXT_PTR(hdrp, NULL);
  CLR_PREV_ALLOC_BIT((char*)hdrp + bk_size); // clear the prev_alloc bit in the next block
}

void insert_into_size_class(void *hdrp, int index) {
#ifdef __LIFO_ORDERING__
  SET_NEXT_PTR(hdrp, segregated_free_list[index]); // hdrp->next = head
  SET_PREV_PTR(hdrp, NULL); // hdrp->prev = NULL
  if (segregated_free_list[index]) { // if (head)
    SET_PREV_PTR(segregated_free_list[index], hdrp); // head->prev = hdrp
  }
  segregated_free_list[index] = hdrp; // head = hdrp
#else // address ordering
  void *head = segregated_free_list[index];
  if (!head) {
    segregated_free_list[index] = hdrp;
  } else if (hdrp < head) {
    SET_NEXT_PTR(hdrp, head); // hdrp->next = head
    SET_PREV_PTR(hdrp, NULL); // hdrp->prev = NULL
    SET_PREV_PTR(head, hdrp); // head->prev = hdrp
    segregated_free_list[index] = hdrp; // head = hdrp
  } else {
    // while (head->next && head->next < hdrp), loop invariant: head < hdrp
    while (GET_NEXT_PTR(head) && GET_NEXT_PTR(head) < hdrp) {
      head = GET_NEXT_PTR(head);
    }
    SET_NEXT_PTR(hdrp, GET_NEXT_PTR(head)); // hdrp->next = head->next
    SET_NEXT_PTR(head, hdrp);               // head->next = hdrp
    SET_PREV_PTR(hdrp, head);               // hdrp->prev = head
    if (GET_NEXT_PTR(hdrp)) {               // if (hdrp->next)
      SET_PREV_PTR(GET_NEXT_PTR(hdrp), hdrp); // hdrp->next->prev = hdrp
    }
  }
#endif
}

// O(1)
void delete_from_size_class(void *hdrp, int index) {
  void *head = segregated_free_list[index];

#ifdef __HEAP_CHECK__
  assert(index >= 0 && index < index_arr_size);
  assert(hdrp);
  assert(head);
  assert(is_in_size_class(hdrp, index));
  assert(hdrp);
  int i = 0;
  for (i = 0; i < index_arr_size; ++i) {
    if (i == index) continue;
    assert(!is_in_size_class(hdrp, i));
  }
#endif

  void *prev_ptr = GET_PREV_PTR(hdrp);
  void *next_ptr = GET_NEXT_PTR(hdrp);
  if (!prev_ptr) { // if current node is the firt node
    segregated_free_list[index] = next_ptr; // head = curr_node->next
  } else {
    SET_NEXT_PTR(prev_ptr, next_ptr); // curr->prev->next = curr_node->next
  }

  if (next_ptr) { // if curr_node->next != NULL
    SET_PREV_PTR(next_ptr, prev_ptr); // curr_node->next->prev = curr_node->prev
  }

  SET_PREV_PTR(hdrp, NULL); // curr->next = NULL
  SET_NEXT_PTR(hdrp, NULL); // curr->prev = NULL
}

int is_in_size_class(void *hdrp, int index) {
#ifdef __HEAP_CHECK__
  assert(hdrp);
#endif
  void *head = segregated_free_list[index];
  while (head && head != hdrp) {
    head = GET_NEXT_PTR(head);
  }
  return head == hdrp ? 1 : 0;
}

void print_list(void *hdrp) {
  fprintf(stderr, "\n\n");
  while (hdrp) {
    fprintf(stderr, "-------------------\n");
    fprintf(stderr, "Block Header: %x, Size = %u, ALLOC Bit = %d\n", hdrp, GET_SIZE(hdrp), GET_ALLOC(hdrp));
    fprintf(stderr, "Previous Ptr: %x, Next Ptr: %x\n", GET_PREV_PTR(hdrp), GET_NEXT_PTR(hdrp));
    hdrp = GET_NEXT_PTR(hdrp);
  }
}

#ifdef __HEAP_CHECK__
void to_binary_str(size_t num, int sep) {
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

void to_hex_str(size_t num, int sep) {
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

void add_to_alloc_list(const void *ptr, const size_t pl_size, const size_t bk_size) {
  if (ptr == NULL) return;
  assert(!addr_is_allocated(ptr));
  assert(search_list(alloc_list, ptr) == NULL);
  assert(pl_size < bk_size);

  HeapStruct* new_node = (HeapStruct*)malloc(sizeof(HeapStruct));
  new_node->bk_head = HDRP_USE_PLDP(ptr);
  new_node->bk_tail = (char*)HDRP_USE_PLDP(ptr) + bk_size;
  new_node->pl_head = ptr;
  new_node->pl_tail = (char*)ptr + pl_size;
  new_node->bk_size = bk_size;
  new_node->pl_size = pl_size;
  new_node->next = alloc_list; // insert it in the front of the list
  new_node->index = -1;
  alloc_list = new_node;

  assert(new_node->pl_tail - new_node->pl_head == pl_size);
  assert(new_node->bk_tail - new_node->bk_head == bk_size);
  assert(new_node->bk_head < new_node->pl_head);
  assert(new_node->bk_tail >= new_node->pl_tail);
  assert(new_node->pl_tail < (char*)heap_tail);
}

void delete_from_alloc_list(const void *ptr) {
  assert(ptr);
  HeapStruct *node = delete_from_list(alloc_list, ptr);
  assert(node);
  free(node);
}

HeapStruct* delete_from_list(HeapStruct *list, const void *ptr) {
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

HeapStruct* search_list(const HeapStruct *list, const void *ptr) {
  assert(ptr);
  while (list && list->pl_head != ptr) {
    list = list->next;
  }
  return list;
}

int addr_is_allocated(const char *addr) {
  HeapStruct *alist = alloc_list;
  while (alist) {
    if (addr >= alist->bk_head && addr < alist->bk_tail) {
      return 1;
    }
    alist = alist->next;
  }
  return 0;
}

int addr_is_payload(const char *addr) {
  HeapStruct *alist = alloc_list;
  while (alist) {
    if ((addr >= alist->pl_head) && (addr < alist->pl_tail)) {
      return 1;
    }
    alist = alist->next;
  }
  return 0;
}

int within_heap(const void *addr) {
  return (addr >= heap_head && addr < heap_tail);
}

void show_heap(void) {
  // TODO: change the code, since i change the heap_tail to one byte after
  void* p = heap_head;
  DebugStr("-----------------\n");
  DebugStr("heap_head = %x, heap size = %u\n", p, (char*)heap_tail  - (char*)heap_head);
  DebugStr("-----------------\n");
  DebugStr("%x\n", (char*)p + WSIZE);
  DebugStr("%x\n", (char*)p + 2 * WSIZE);
  DebugStr("-----------------\n");
  p = (char*)p + 3 * WSIZE;
  assert(p < heap_tail);
  while (GET_SIZE(p) > 0) {
    assert(p < heap_tail);
    DebugStr("hdrp:%x val = ", p);
    to_hex_str(*(size_t*)p, 1);
    DebugStr("size = %u, ALLOC = %d, PREV_ALLOC = %d\n", GET_SIZE(p), !!GET_ALLOC(p), !!GET_PREV_ALLOC(p));
    void *ftrp = (char*)p + GET_SIZE(p) - WSIZE;
    DebugStr("ftrp:%x val = ", ftrp);
    to_hex_str(*(size_t*)ftrp, 1);
    if (!GET_ALLOC(p)) { // if current block is not allocated, header = footer
      assert(READ_WORD(p) == READ_WORD(ftrp));
    }
    DebugStr("-----------------\n");
    p += GET_SIZE(p);
  }
  DebugStr("heap_tail = %x\n", p);
  DebugStr("-----------------\n");
}

handler_t *Signal(int signum, handler_t *handler) {
  struct sigaction action, old_action;

  action.sa_handler = handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = SA_RESTART;

  if (sigaction(signum, &action, &old_action) < 0) {
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }
  return old_action.sa_handler;
}

void show_alloc_list(void) {
  HeapStruct *alist = alloc_list;
  DebugStr("\n-------------------\n");
  while (alist) {
    DebugStr("Head = %x, Tail = %x, Payload Head = %x, Payload Tail = %x\n",
      alist->bk_head, alist->bk_tail, alist->pl_head, alist->pl_tail);
    DebugStr("-------------------\n");
    alist = alist->next;
  }
}

void print_stack_trace(int signum) {
  void *array[NUM_STACK_TRACE];
  size_t size;
  char **strings;
  size_t i;
  size = backtrace(array, NUM_STACK_TRACE);
  strings = backtrace_symbols(array, size);
  for (i = 1; i < size; ++i) {
    fprintf(stderr, "%s\n", strings[i]);
  }
  free(strings);
}

size_t heap_size(void) {
  return (char*)heap_tail - (char*)heap_head;
}

int segregated_free_list_valid(void) {
  int i = 0;
  for (i = 0; i < index_arr_size; ++i) {
    size_t low = index_array[i];
    size_t high = low * 2 - 1;
    void *head = segregated_free_list[i];
    while (head) {
      size_t size = GET_SIZE(head);
      assert(size >= low && size < high);
      head = GET_NEXT_PTR(head);
    }
  }
}

#endif