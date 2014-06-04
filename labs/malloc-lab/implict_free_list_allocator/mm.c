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

// Macro Operations
// Allocated Memory Structure
// [ 1 W Header | Payload | [Optional Padding] | 1 W Footer]
// hdrp (header pointer) points to the start address of header
// ftrp (footer pointer) points to the start address of footer
// pldp (payload pointer) points to the start address of payload

// generate the header from size and alloc bit
#define PACK(size, alloc)                           ((size) | (alloc))
#define READ_WORD(p)                                (*(uint32_t*)(p))
#define WRITE_WORD(p, val)                          (*(uint32_t*)(p) = (val))
#define SIZE_MASK                                   (~(ALIGNMENT - 1))
#define GET_SIZE(hdrp)                              (READ_WORD(hdrp) & SIZE_MASK)  // get block size from hdrp
#define GET_ALLOC(hdrp)                             (READ_WORD(hdrp) & 0x1)  // get alloc bit from hdrp
#define HDRP_USE_PLDP(pldp)                         ((char*)(pldp) - WSIZE)  // get hdrp using pldp
#define FTRP_USE_PLDP(pldp)                         ((char*)(pldp) + GET_SIZE(HDRP_USE_PLDP(pldp)) - DSIZE)  // get ftrp using pldp

#define PREV_FTRP_USE_PLDP(pldp)                    ((char*)HDRP_USE_PLDP(pldp) - WSIZE)  // get previous block footer using current pldp
#define NEXT_HDRP_USE_PLDP(pldp)                    (HDRP_USE_PLDP((char*)(pldp) + GET_SIZE(HDRP_USE_PLDP(pldp))))  // get the next block header using current pldp

#define NEXT_PLDP_USE_PLDP(pldp)                    ((char*)(pldp) + GET_SIZE(HDRP_USE_PLDP(pldp)))  // get next block pldp using current pldp
#define PREV_PLDP_USE_PLDP(pldp)                    ((char*)(pldp) - GET_SIZE(PREV_FTRP_USE_PLDP(pldp)))  // get previous block pldp using current pldp
#define NEXT_PLDP_USE_HDRP(hdrp)                    ((char*)(hdrp) + GET_SIZE(hdrp) + WSIZE)  // get the next pldp using current hdrp

static void* heap_listp = NULL;
// Function Declaration
static void *extend_heap(size_t size);
static void *coalesce(void *pldp);
// the input size should be aligned
static void *find_first_fit(size_t asize);
// the input size should be aligned
static void place_and_split(void *pldp, size_t asize);

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
  WRITE_WORD(heap_listp, 0);
  WRITE_WORD(heap_listp + WSIZE, PACK(DSIZE, 1));           // Prologue header
  WRITE_WORD(heap_listp + 2 * WSIZE, PACK(DSIZE, 1));       // Prologue footer
  WRITE_WORD(heap_listp + 3 * WSIZE, PACK(0, 1));           // Epilogue header
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
  size_t asize = ALIGN(size + WSIZE);
  void *pldp = find_first_fit(asize);
  if (pldp) {
    place_and_split(pldp, asize);
    return pldp;
  }
  if (!(pldp = extend_heap(asize))) {
    return NULL;
  }
  place_and_split(pldp, asize);
  return pldp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *pldp) {
  size_t size = GET_SIZE(HDRP_USE_PLDP(pldp));
  WRITE_WORD(HDRP_USE_PLDP(pldp), PACK(size, 0));
  WRITE_WORD(FTRP_USE_PLDP(pldp), PACK(size, 0));
  coalesce(pldp); // immediate coalescing
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
  void *oldptr = ptr;
  void *newptr;
  size_t copySize;

  newptr = mm_malloc(size);
  if (newptr == NULL) return NULL;
  copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
  if (size < copySize) copySize = size;
  memcpy(newptr, oldptr, copySize);
  mm_free(oldptr);
  return newptr;
}

static void *extend_heap(size_t size) {
  char *hdrp;
  size_t asize = ALIGN_CHUNKSIZE(size);
  if ((void*)(hdrp = mem_sbrk(asize)) == (void*)(-1)) return NULL;
  hdrp -= WSIZE; // points to the old Epilogue header
  WRITE_WORD(hdrp, PACK(asize, 0)); // header
  WRITE_WORD(hdrp + asize - WSIZE, PACK(asize, 0)); // footer
  WRITE_WORD(NEXT_HDRP_USE_PLDP(hdrp + WSIZE), PACK(0, 1)); // write new epilogue block

  return coalesce(hdrp + WSIZE);
}

static void *coalesce(void *pldp) {
  size_t prev_alloc = GET_ALLOC(PREV_FTRP_USE_PLDP(pldp));
  size_t next_alloc = GET_ALLOC(NEXT_HDRP_USE_PLDP(pldp));
  size_t size = GET_SIZE(HDRP_USE_PLDP(pldp));

  // previous and next are both allocated
  if (prev_alloc && next_alloc) {
    return pldp;
  }

  // previous is free but next is allocated
  void *prev_pldp = PREV_PLDP_USE_PLDP(pldp);
  if (!prev_alloc && next_alloc) {
    size += GET_SIZE(HDRP_USE_PLDP(prev_pldp)); // add previous block's size
    WRITE_WORD(HDRP_USE_PLDP(prev_pldp), PACK(size, 0)); // write new header in previous block
    WRITE_WORD(FTRP_USE_PLDP(pldp), PACK(size, 0)); // write new footer in current block
    return prev_pldp;
  }

  // next is free but previous is allocated
  void *next_pldp = NEXT_PLDP_USE_PLDP(pldp);
  if (prev_alloc && !next_alloc) {
    size += GET_SIZE(HDRP_USE_PLDP(next_pldp)); // add next block's size
    WRITE_WORD(HDRP_USE_PLDP(pldp), PACK(size, 0)); // write new header in current block
    WRITE_WORD(FTRP_USE_PLDP(next_pldp), PACK(size, 0)); // write new footer in next block
    return pldp;
  }

  // previous and next are both free
  size += GET_SIZE(HDRP_USE_PLDP(prev_pldp)) + GET_SIZE(HDRP_USE_PLDP(next_pldp));
  WRITE_WORD(HDRP_USE_PLDP(prev_pldp), PACK(size, 0)); // write new header in previous block
  WRITE_WORD(FTRP_USE_PLDP(next_pldp), PACK(size, 0)); // write new footer in next block
  return prev_pldp;
}

// linear search
static void *find_first_fit(size_t asize) {
  void *hdrp = heap_listp;
  size_t b_size;
  do {
    b_size = GET_SIZE(hdrp);
    if (asize <= b_size && !GET_ALLOC(hdrp)) {
      return hdrp + WSIZE;
    }
    hdrp += b_size;
  } while (b_size > 0);
  return NULL;
}

static void place_and_split(void *pldp, size_t asize) {
  size_t b_size = GET_SIZE(HDRP_USE_PLDP(pldp));
  size_t left_size = b_size - asize;
  if (left_size && IS_ALIGN_WITH_MIN_BK_SIZE(left_size)) { // reminder > 0 and is multiple of minimum blockasize
    void *new_pldp = pldp + asize;
    WRITE_WORD(HDRP_USE_PLDP(new_pldp), PACK(left_size, 0));
    WRITE_WORD(FTRP_USE_PLDP(new_pldp), PACK(left_size, 0));
    b_size = asize;
  }
  WRITE_WORD(HDRP_USE_PLDP(pldp), PACK(b_size, 1));
  WRITE_WORD(FTRP_USE_PLDP(pldp), PACK(b_size, 1));
}
