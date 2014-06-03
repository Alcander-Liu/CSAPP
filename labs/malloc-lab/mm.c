// TODO:
// 1. try implicit list
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
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

// if complie with -m32, sizeof(size_t) = 4
// if complie with -m64, sizeof(size_t) = 8
// but in either case, ALIGN(sizeof(size_t)) = 8
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE               4
#define DSIZE               8
#define CHUNKSIZE           (1 << 12)               // equal the page size 4kb

// Macro Operations
#define PACK(size, alloc)                           ((size) | (alloc))
#define READ_WORD(p)                                (*(uint32_t*)(p))
#define WRITE_WORD(p, val)                          (*(uint32_t*)(p) = (val))
#define SIZE_MASK                                   (~(ALIGNMENT - 1))
#define GET_SIZE(p)                                 (READ_WORD(p) & SIZE_MASK)
#define GET_ALLOC(p)                                (READ_WORD(p) & 0x1)
#define HDRP(bp)                                    ((char*)(bp) - WSIZE)
#define FTRP(bp)                                    ((char*)(bp) + GET_SIZE(HDRP(bp)) - WSIZE)
#define NEXT_BLKP(bp)                               ((char*)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp)                               ((char*)(bp) - GET_SIZE((char*)(bp) - WSIZE))

static void* heap_listp = NULL;

// Function Declaration
static void *extend_heap(size_t size);
static void *coalesce(void *bp);


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
  heap_listp += 2 * WSIZE;

  if (extend_heap(CHUNKSIZE) == NULL) {
    return -1;
  }
  return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
  int newsize = ALIGN(size + SIZE_T_SIZE);
  void *p = mem_sbrk(newsize);
  if (p == (void *)-1) return NULL;
  else {
    *(size_t *)p = size;
    return (void *)((char *)p + SIZE_T_SIZE);
  }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
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
  char *bp;
  size = ALIGN(size);

  if ((void*)(bp = mem_sbrk(size)) == (void*)-1) return NULL;

  WRITE_WORD(HDRP(bp), PACK(size, 0));          // Free block header
  WRITE_WORD(FTRP(bp), PACK(size, 0));          // Free block footer
  WRITE_WORD(NEXT_BLKP(bp), PACK(0, 1));        // epilogue header

  return coalesce(bp);
}

static void *coalesce(void *bp) {
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  // previous and next are both allocated
  if (prev_alloc && next_alloc) {
    return NULL;
  }

  // previous is free but next is allocated
  void *prev_bp = PREV_BLKP(bp);
  if (!prev_alloc && next_alloc) {
    size += GET_SIZE(HDRP(prev_bp));
    WRITE_WORD(HDRP(prev_bp), PACK(size, 0));
    WRITE_WORD(FTRP(bp), PACK(size, 0));
    return prev_bp;
  }

  // next is free but previous is allocated
  void *next_bp = NEXT_BLKP(bp);
  if (prev_alloc && !next_alloc) {
    size += GET_SIZE(HDRP(next_bp));
    WRITE_WORD(HDRP(bp), PACK(size, 0));
    WRITE_WORD(FTRP(next_bp), PACK(size, 0));
    return bp;
  }

  // previous and next are both free
  size += GET_SIZE(HDRP(prev_bp)) + GET_SIZE(HDRP(next_bp));
  WRITE_WORD(HDRP(prev_bp), PACK(size, 0));
  WRITE_WORD(FTRP(next_bp), PACK(size, 0));
  return prev_bp;
}
