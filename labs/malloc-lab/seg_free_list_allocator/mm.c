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

#define LIFO_ORDERING       1
#define BINARY_OPT          1
#define FIRST_FIT           0
#define HEAP_CHECK          0
#define LOG_TO_STDERR       0

#define NUM_STACK_TRACE     (20)
#define ALIGNMENT           (8)
#define MIN_BK_SIZE         (16)
#define WSIZE               (4)
#define DSIZE               (8)
#define CHUNKSIZE           (176)
#define REALLOC_CHUNKSIZE   (304)

static void* heap_head = NULL; // points to heap start
static void* heap_tail = NULL; // points to heap end, one byte after Epilogue header

#include "mm_helper.h"

const static int LEVEL = 28;
static size_t *free_list_arr = NULL;
static size_t *index_arr = NULL;

void *extend_heap(size_t size, int type);
void *realloc_extend_heap(size_t size);
void *coalesce(void *hdrp);
void *find_fit(size_t asize);
void *place_and_split(void *hdrp, size_t asize);
void *realloc_place_and_split(void *hdrp, size_t asize);
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

#if HEAP_CHECK
  Signal(SIGABRT, print_stack_trace);
  heap_head = NULL;
  heap_tail = NULL;
  alloc_list = NULL;
#endif
  if (!(free_list_arr = mem_sbrk(LEVEL * WSIZE))) {
    return NULL;
  }

  if (!(index_arr = mem_sbrk(LEVEL * WSIZE))) {
    return NULL;
  }

  int i = 0;
  for (; i < LEVEL; ++i) {
    *(index_arr + i) = 1 << (i + 4);
    *(free_list_arr + i) = NULL;
  }

  // 1 WSIZE for heap-start padding
  // 1 WSIZE for heap-end padding
  void *temp = NULL;
  if ((temp = mem_sbrk(2 * WSIZE)) == (void*)(-1)) {
    return -1;
  }
  heap_head = (void*)free_list_arr;
  heap_tail = (char*)temp + 2 * WSIZE;
  write_word(temp, pack(0, CURR_ALLOC));  // heap-start 0 padding
  write_word((char*)temp + WSIZE, pack(0, PREV_ALLOC | CURR_ALLOC));  // heap-end 0 padding


#if HEAP_CHECK
  assert(heap_size() == (2 * LEVEL) * WSIZE + 2 * WSIZE);
#endif
  return extend_heap(CHUNKSIZE, 0) == NULL ? -1 : 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 */
void *mm_malloc(size_t size) {
  if (!size) return NULL;

  void *ret = NULL;
  size_t asize = align_with_min_bk_size(size + WSIZE);
  void *head = find_fit(asize);
  if (head) {

#if HEAP_CHECK
    assert(within_heap(head));
    assert(!addr_is_allocated(head));
    assert(!addr_is_payload(head));
#endif

    head = place_and_split(head, asize);
    ret = (char*)head + WSIZE;
  } else {
    void *heap_end_padding = (char*)heap_tail - WSIZE;
    if (!get_prev_alloc(heap_end_padding)) {
      void *footer = (char*)heap_end_padding - WSIZE;
      size_t end_bk_size = get_size(footer);
      head = extend_heap(asize - end_bk_size, 0);
    } else {
      head = extend_heap(asize, 0);
    }
    head = place_and_split(head, asize);
    ret = (char*)head + WSIZE;
  }

#if HEAP_CHECK
  add_to_alloc_list(ret, size, asize);
#endif

  return ret;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
#if HEAP_CHECK
  assert(ptr);
  delete_from_alloc_list(ptr);
#endif
  void *head = (char*)ptr - WSIZE;
  size_t size = get_size(head);
  init_free_block(head, size);
  insert_into_size_class(head, find_index(size));
  coalesce(head);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
  if (!ptr) {
    return mm_malloc(size);
  }

  if (!size) {
    mm_free(ptr);
    return NULL;
  }

  void *hdrp = get_hdrp(ptr);
  size_t old_size = get_size(hdrp);
  size_t ori_size = old_size;
  size_t target_size = align_with_min_bk_size(size + WSIZE);
  // if possible, we won't move the data, collect space in place
  hdrp = backward_collect(hdrp, target_size);
  old_size = get_size(hdrp);
  if (target_size <= old_size) {
#if HEAP_CHECK
    delete_from_alloc_list(ptr);
#endif
    size_t left_size = old_size - target_size;
    if (left_size && is_align_with_min_bk_size(left_size)) {
      set_size(hdrp, target_size);
      set_alloc_bit(hdrp);
      void *new_free_hdrp = (char*)hdrp + target_size;
      write_word(new_free_hdrp, PREV_ALLOC);
      init_free_block(new_free_hdrp, left_size);
      insert_into_size_class(new_free_hdrp, find_index(left_size));
    } else {
      set_alloc_bit(hdrp);
      set_prev_alloc_bit((char*)hdrp + old_size);
    }
#if HEAP_CHECK
    add_to_alloc_list(ptr, size, target_size);
#endif
    return (char*)hdrp + WSIZE;
  }

  // find fit
  void *new_hdrp = find_fit(target_size);
  if (new_hdrp) {
    new_hdrp = place_and_split(new_hdrp, target_size);
    mm_memcpy((char*)new_hdrp + WSIZE, ptr, ori_size - WSIZE);
    mm_free(ptr);
#if HEAP_CHECK
    add_to_alloc_list((char*)new_hdrp + WSIZE, size, target_size);
#endif
    return (char*)new_hdrp + WSIZE;
  }

  // extend heap
  new_hdrp = extend_heap(target_size, 1);
  if (!new_hdrp) return NULL;
  new_hdrp = place_and_split(new_hdrp, target_size);
  mm_memcpy((char*)new_hdrp + WSIZE, ptr, ori_size - WSIZE);
  mm_free(ptr);
#if HEAP_CHECK
  add_to_alloc_list((char*)new_hdrp + WSIZE, size, target_size);
#endif
  return (char*)new_hdrp + WSIZE;
}

int find_index(size_t size) {
  int low = 0, high = LEVEL;
  int ret = -1;
  int mid;
  while (low < high) {
    mid = (high + low) >> 1;
    if (size < index_arr[mid]) {
      high = mid;
    } else {
      low = mid + 1;
    }
  }
  return low - 1;
}

void *extend_heap(size_t size, int type) {
  char *hdrp = NULL;
  size_t asize =
    type == 0 ? align_chunksize(size) : align_realloc_chunksize(size);
  if ((void*)(hdrp = mem_sbrk(asize)) == (void*)(-1)) return NULL;
  heap_tail += asize;
  hdrp = (char*)hdrp - WSIZE; // points to heap-end 0 padding
  init_free_block(hdrp, asize); // pack header and footer
  insert_into_size_class(hdrp, find_index(asize));
  write_word((char*)hdrp + asize, pack(0, 1)); // write new heap-end padding
  return coalesce((char*)hdrp);
}

void *coalesce(void *hdrp) {
  size_t size = get_size(hdrp);
  size_t prev_alloc = get_prev_alloc(hdrp); // read current header to get the privous block allocate bit
  size_t next_alloc = get_alloc((char*)hdrp + size); // read next block header to get its allocate bit
  // previous and next are both allocated
  if (prev_alloc && next_alloc) {
    return hdrp;
  }
  // previous is free but next is allocated
  if (!prev_alloc && next_alloc) {
    size_t prev_bk_size = get_size((char*)hdrp - WSIZE);
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
    size_t next_bk_size = get_size(next_hdrp);
    delete_from_size_class(next_hdrp, find_index(next_bk_size));
    delete_from_size_class(hdrp, find_index(size));
    size_t new_size = next_bk_size + size;
    init_free_block(hdrp, new_size);
    insert_into_size_class(hdrp, find_index(new_size));
    return hdrp;
  }
  // previous and next are both free
  size_t prev_bk_size = get_size((char*)hdrp - WSIZE);
  void *prev_hdrp = (char*)hdrp - prev_bk_size;
  void *next_hdrp = (char*)hdrp + size;
  size_t next_bk_size = get_size(next_hdrp);
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
#if FIRST_FIT
  int index = find_index(asize);
  while (index < LEVEL) {
    void *head = free_list_arr[index];
    while (head) {
      if (get_size(head) >= asize) {
        return head;
      }
      head = get_next_ptr(head);
    }
    ++index;
  }
#else
  int index = find_index(asize);
  void *ret = NULL;
  size_t min_size = (1 << 31) - 1;
  while (index < LEVEL) {
    void *head = free_list_arr[index];
    while (head) {
      size_t h_size = get_size(head);
      if (h_size >= asize && h_size < min_size) {
        ret = head;
        min_size = h_size;
      }
      head = get_next_ptr(head);
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
  size_t bk_size = get_size(hdrp);
#if HEAP_CHECK
  assert(bk_size >= asize);
#endif
  delete_from_size_class(hdrp, find_index(bk_size));
  size_t left_size = bk_size - asize;
  // pack small blocks in the front part of the heap
#if BINARY_OPT
  if (left_size >= bk_size) {
#endif
    if (left_size && is_align_with_min_bk_size(left_size)) {
      set_size(hdrp, asize);
      set_alloc_bit(hdrp);
      void *new_free_hdrp = (char*)hdrp + asize;
      write_word(new_free_hdrp, PREV_ALLOC);
      init_free_block(new_free_hdrp, left_size);
      insert_into_size_class(new_free_hdrp, find_index(left_size));
    } else {
      set_alloc_bit(hdrp);
      set_prev_alloc_bit((char*)hdrp + bk_size);
    }
    return hdrp;
#if BINARY_OPT
  } else {
    if (left_size && is_align_with_min_bk_size(left_size)) {
      set_size(hdrp, left_size);
      init_free_block(hdrp, left_size);
      insert_into_size_class(hdrp, find_index(left_size));

      void *new_free_hdrp = (char*)hdrp + left_size;
      write_word(new_free_hdrp, pack(asize, 1));
      set_prev_alloc_bit((char*)new_free_hdrp + asize);
      return new_free_hdrp;
    } else {
      set_alloc_bit(hdrp);
      set_prev_alloc_bit((char*)hdrp + bk_size);
      return hdrp;
    }
  }
#endif
}

void *backward_collect(void *hdrp, size_t target_size) {
  void *next_hdrp = (char*)hdrp + get_size(hdrp);
  while (get_size(hdrp) < target_size && !get_alloc(next_hdrp)) {
    size_t next_size = get_size(next_hdrp);
    delete_from_size_class(next_hdrp, find_index(next_size));
    write_word(hdrp, read_word(hdrp) + next_size); // update header
    next_hdrp = (char*)next_hdrp + next_size;
    set_prev_alloc_bit(next_hdrp);
  }

  size_t size = get_size(hdrp);
  if (size < target_size &&
      (char*)hdrp + size == (char*)heap_tail - WSIZE) {
    hdrp = extend_heap(target_size - size, 1);
  }
  return hdrp;
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
  write_word(hdrp, pack(bk_size, get_prev_alloc(hdrp))); // write new header and keep prev_alloc bit
  write_word((char*)hdrp + bk_size - WSIZE, read_word(hdrp)); // write footer
  set_prev_ptr(hdrp, NULL);
  set_next_ptr(hdrp, NULL);
  clr_prev_alloc_bit((char*)hdrp + bk_size); // clear the prev_alloc bit in the next block
}

void insert_into_size_class(void *hdrp, int index) {
#if LIFO_ORDERING
  set_next_ptr(hdrp, free_list_arr[index]); // hdrp->next = head
  set_prev_ptr(hdrp, NULL); // hdrp->prev = NULL
  if (free_list_arr[index]) { // if (head)
    set_prev_ptr(free_list_arr[index], hdrp); // head->prev = hdrp
  }
  free_list_arr[index] = hdrp; // head = hdrp
#else // address ordering
  void *head = free_list_arr[index];
  if (!head) {
    free_list_arr[index] = hdrp;
  } else if (hdrp < head) {
    set_next_ptr(hdrp, head); // hdrp->next = head
    set_prev_ptr(hdrp, NULL); // hdrp->prev = NULL
    set_prev_ptr(head, hdrp); // head->prev = hdrp
    free_list_arr[index] = hdrp; // head = hdrp
  } else {
    // while (head->next && head->next < hdrp), loop invariant: head < hdrp
    while (get_next_ptr(head) && get_next_ptr(head) < hdrp) {
      head = get_next_ptr(head);
    }
    set_next_ptr(hdrp, get_next_ptr(head)); // hdrp->next = head->next
    set_next_ptr(head, hdrp);               // head->next = hdrp
    set_prev_ptr(hdrp, head);               // hdrp->prev = head
    if (get_next_ptr(hdrp)) {               // if (hdrp->next)
      set_prev_ptr(get_next_ptr(hdrp), hdrp); // hdrp->next->prev = hdrp
    }
  }
#endif
}

// O(1)
void delete_from_size_class(void *hdrp, int index) {
  void *head = free_list_arr[index];
#if HEAP_CHECK
  assert(index >= 0 && index < LEVEL);
  assert(hdrp);
  assert(head);
  assert(is_in_size_class(hdrp, index));
  assert(hdrp);
  int i = 0;
  for (i = 0; i < LEVEL; ++i) {
    if (i == index) continue;
    assert(!is_in_size_class(hdrp, i));
  }
#endif
  void *prev_ptr = get_prev_ptr(hdrp);
  void *next_ptr = get_next_ptr(hdrp);
  if (!prev_ptr) { // if current node is the firt node
    free_list_arr[index] = next_ptr; // head = curr_node->next
  } else {
    set_next_ptr(prev_ptr, next_ptr); // curr->prev->next = curr_node->next
  }

  if (next_ptr) { // if curr_node->next != NULL
    set_prev_ptr(next_ptr, prev_ptr); // curr_node->next->prev = curr_node->prev
  }
  set_prev_ptr(hdrp, NULL); // curr->next = NULL
  set_next_ptr(hdrp, NULL); // curr->prev = NULL
}

int is_in_size_class(void *hdrp, int index) {
  void *head = free_list_arr[index];
  while (head && head != hdrp) {
    head = get_next_ptr(head);
  }
  return head == hdrp ? 1 : 0;
}

void print_list(void *hdrp) {
  fprintf(stderr, "\n\n");
  while (hdrp) {
    fprintf(stderr, "-------------------\n");
    fprintf(stderr, "Block Header: %x, Size = %u, ALLOC Bit = %d\n",
            hdrp, get_size(hdrp), get_alloc(hdrp));
    fprintf(stderr, "Previous Ptr: %x, Next Ptr: %x\n",
            get_prev_ptr(hdrp), get_next_ptr(hdrp));
    hdrp = get_next_ptr(hdrp);
  }
}

// --------------------------------------
// Debug Helper Functions
// --------------------------------------
#if HEAP_CHECK

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

void add_to_alloc_list(const void *ptr,
                       const size_t pl_size,
                       const size_t bk_size) {
  if (ptr == NULL) return;
  assert(!addr_is_allocated(ptr));
  assert(search_list(alloc_list, ptr) == NULL);
  assert(pl_size < bk_size);
  HeapStruct* new_node = (HeapStruct*)malloc(sizeof(HeapStruct));
  new_node->bk_head = get_hdrp(ptr);
  new_node->bk_tail = (char*)get_hdrp(ptr) + bk_size;
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
  DebugStr("heap_head = %x, heap size = %u\n",
           p, (char*)heap_tail  - (char*)heap_head);
  DebugStr("-----------------\n");
  DebugStr("%x\n", (char*)p + WSIZE);
  DebugStr("%x\n", (char*)p + 2 * WSIZE);
  DebugStr("-----------------\n");
  p = (char*)p + 3 * WSIZE;
  assert(p < heap_tail);
  while (get_size(p) > 0) {
    assert(p < heap_tail);
    DebugStr("hdrp:%x val = ", p);
    to_hex_str(*(size_t*)p, 1);
    DebugStr("size = %u, ALLOC = %d, PREV_ALLOC = %d\n",
             get_size(p), !!get_alloc(p), !!get_prev_alloc(p));
    void *ftrp = (char*)p + get_size(p) - WSIZE;
    DebugStr("ftrp:%x val = ", ftrp);
    to_hex_str(*(size_t*)ftrp, 1);
    if (!get_alloc(p)) { // if current block is not allocated, header = footer
      assert(read_word(p) == read_word(ftrp));
    }
    DebugStr("-----------------\n");
    p += get_size(p);
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
  for (i = 0; i < LEVEL; ++i) {
    size_t low = index_arr[i];
    size_t high = low * 2 - 1;
    void *head = free_list_arr[i];
    while (head) {
      size_t size = get_size(head);
      assert(size >= low && size < high);
      head = get_next_ptr(head);
    }
  }
}
#endif