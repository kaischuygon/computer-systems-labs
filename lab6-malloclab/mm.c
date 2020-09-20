/* 
 * mm.c -  Simple allocator based on implicit free lists, 
 *         first fit placement, and boundary tag coalescing. 
 *
 * Each block has header and footer of the form:
 * 
 *      31                     3  2  1  0 
 *      -----------------------------------
 *     | s  s  s  s  ... s  s  s  0  0  a/f
 *      ----------------------------------- 
 * 
 * where s are the meaningful size bits and a/f is set 
 * iff the block is allocated. The list has the following form:
 *
 * begin                                                          end
 * heap                                                           heap  
 *  -----------------------------------------------------------------   
 * |  pad   | hdr(8:a) | ftr(8:a) | zero or more usr blks | hdr(8:a) |
 *  -----------------------------------------------------------------
 *          |       prologue      |                       | epilogue |
 *          |         block       |                       | block    |
 *
 * The allocated prologue and epilogue blocks are overhead that
 * eliminate edge conditions during coalescing.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include "mm.h"
#include "memlib.h"

team_t team = {
  /* Team name */
  "kasc1637",
  /* First member's full name */
  "Kai Schuyler Gonzalez",
  /* First member's email address */
  "kasc1637@colorado.edu",
  /* Second member's full name (leave blank if none) */
  "",
  /* Second member's email address (leave blank if none) */
  ""
};

// CONSTANTS
// #define DEBUG               // uncomment for debugging
#define ALIGNMENT   8       // memory alignment factor
#define WSIZE       4       // word size (bytes)
#define DSIZE       8       // doubleword size (bytes)
#define CHUNKSIZE   16      // initial size of free list before first free block is added
#define MINIMUM     24      // minimum block size
// MACROS
#define ALIGN(p) (((uint32_t)(p) + (ALIGNMENT - 1)) & ~0x7)
static inline int MAX(int x, int y) { return x > y ? x : y; }

// Pack a size and allocated bit into a word
static inline uint32_t PACK(uint32_t size, int alloc) { return ((size) | (alloc & 0x1)); }

// Read and write a word at address p
static inline uint32_t GET(void *p) { return *(uint32_t *)p; }
static inline void PUT(void *p, uint32_t val) { *((uint32_t *)p) = val; }

// Read the size and allocated fields from address p
static inline uint32_t GET_SIZE(void *p) { return GET(p) & ~0x7; }
static inline int GET_ALLOC(void *p) { return GET(p) & 0x1; }

// Given block ptr bp, compute address of its header and footer
static inline void *HDRP(void *bp) { return ((char *)bp) - WSIZE; }
static inline void *FTRP(void *bp) { return ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE); }

// Given block ptr bp, compute address of next and previous blocks
static inline void *NEXT_BLKP(void *bp) { return ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))); }
static inline void* PREV_BLKP(void *bp) { return ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))); }

#define NEXT_FREE(bp) (*(char **)(bp + DSIZE))
#define PREV_FREE(bp) (*(char **)(bp))

// Global Variables:
static char *heap_listp = 0; // pointer to first block
static char *free_listp = 0; // pointer to start of free block

// function prototypes for internal helper routines
static void *extend_heap(uint32_t words);
static void place(void *bp, uint32_t asize);
static void *find_fit(uint32_t asize);
static void *coalesce(void *bp);
static void insertBlock(void *bp); // free_listp function
static void removeBlock(void *bp); // free_listp function

// mm_init - Initialize the memory manager 
int mm_init(void) {
  if((heap_listp = mem_sbrk(2 * MINIMUM)) == (void *)-1)
    return -1;
  PUT(heap_listp, 0); // Alignment padding

  // initialize block header
  PUT(heap_listp + WSIZE, PACK(MINIMUM, 1)); // WSIZE = padding
  PUT(heap_listp + DSIZE, 0); // PREV pointer
  PUT(heap_listp + DSIZE + WSIZE, 0); // Next pointer

  // initialize block footer
  PUT(heap_listp + MINIMUM, PACK(MINIMUM, 1));

  // initialize tail block
  PUT(heap_listp + WSIZE + MINIMUM, PACK(0, 1));

  // initialize free list pointer to tail block
  free_listp = heap_listp + DSIZE;

  // return -1 if unable to get heap space
  if(extend_heap(CHUNKSIZE / WSIZE) == NULL)
    return -1;
  
  return 0;
}

// extend_heap - Extend heap with free block and return its block pointer
static void *extend_heap(uint32_t words) {
  char *bp;
  uint32_t asize;
  // adjust size so alignment and minimum block size reqs. are met
  asize = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
  if(asize < MINIMUM)
    asize = MINIMUM;

  // grow heap by adj size
  if((long)(bp = mem_sbrk(asize)) == -1)
    return NULL;

  // set header and footer of newly created free block and push epilogue header to back
  PUT(HDRP(bp), PACK(asize, 0)); // free block header
  PUT(FTRP(bp), PACK(asize, 0)); // free block footer
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // move epilogue to end
  
  //coalesce any partitioned memory
  return coalesce(bp);
}

// find_fit - Find a fit for a block with asize bytes 
static void *find_fit(uint32_t asize) {
  void *bp;
  // first fit search
  for(bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_FREE(bp)) {
    if(asize <= GET_SIZE(HDRP(bp)))
      return bp;
  }
  // otherwise no free block was large enough
  return NULL;
}

// mm_free - Free a block 
void mm_free(void *bp) {
  if(!bp) // ignore spurious requests
    return;
  uint32_t size = GET_SIZE(HDRP(bp));

  // set header and footer alloc bits to 0 to free block
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));

  // coalesce to merge any free blocks and add to list
  coalesce(bp);
}

// coalesce - boundary tag coalescing. Return ptr to coalesced block
static void *coalesce(void *bp) {
  // determine current alloc state of previous and next blocks
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || PREV_BLKP(bp) == bp;
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  // get size of current free block
  size_t size = GET_SIZE(HDRP(bp));

  // if next block is free, coalesce current block bp and next block
  if(prev_alloc & !next_alloc) { // case 1
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    removeBlock(NEXT_BLKP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }
  // if prev block is free, coalesce current block and prev block
  else if(!prev_alloc && next_alloc) { // case 2
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    bp = PREV_BLKP(bp);
    removeBlock(bp);
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }

  // if previous block and next block are free, coalesce both
  else if(!prev_alloc && !next_alloc) { // Case 3
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
    removeBlock(PREV_BLKP(bp));
    removeBlock(NEXT_BLKP(bp));
    bp = PREV_BLKP(bp);
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }

  // insert coalsced block at front of free list
  insertBlock(bp);

  // return coalesced block
  return bp;
}

// mm_malloc - Allocate a block with at least size bytes of payload 
void *mm_malloc(uint32_t size) {
  size_t asize;       // adjusted block size
  size_t extendsize;  // Amount to extend heap if no fit
  char *bp;

  // Ignore spurious requests
  if(size <= 0)
    return NULL;
  
  // size of new block is equal to the size of the header and footer, plus the size of payload
  asize = MAX(ALIGN(size) + DSIZE, MINIMUM);

  // search free list for fit
  if((bp = find_fit(asize))) {
    place(bp, asize);
    return bp;
  }

  // otherwise, no fit found, grow heap
  extendsize = MAX(asize, CHUNKSIZE);
  if((bp = extend_heap(extendsize / WSIZE)) == NULL)
    return NULL;

  // place newly allocated block
  place(bp, asize);
  return bp;
} 

// place - Place block of asize bytes at start of free block bp and split if remainder would be at least minimum block size
static void place(void *bp, uint32_t asize) {
  size_t csize = GET_SIZE(HDRP(bp));
  if((csize - asize) >= (MINIMUM)) { // case 1: splitting
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    removeBlock(bp);
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize - asize, 0));
    PUT(FTRP(bp), PACK(csize - asize, 0));
    coalesce(bp);
  }
  else { // case 2: splitting not possible, use full free block
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
    removeBlock(bp);
  }
}

// mm_realloc - reallocated a block of memory
void *mm_realloc(void *ptr, uint32_t size) {
  uint32_t oldsize;
  void *newptr;
  uint32_t asize = MAX(ALIGN(size) + DSIZE, MINIMUM);
  // if size <= 0, just free and return NULL
  if(size <= 0) {
    mm_free(ptr);
    return 0;
  }

  // if oldptr is NULL, then malloc
  if(ptr == NULL) {
    return mm_malloc(size);
  }

  // get size of original block
  oldsize = GET_SIZE(HDRP(ptr));

  // if size doesn't need to be changed
  if(asize == oldsize)
    return ptr;

  // if size needs to be decreased, shrink block and return same pointer
  if(asize <= oldsize) {
    size = asize;
    // if new block can't fit in remaining space, return pointer
    if(oldsize - size <= MINIMUM)
      return ptr;
    PUT(HDRP(ptr), PACK(size, 1));
    PUT(FTRP(ptr), PACK(size, 1));
    PUT(HDRP(NEXT_BLKP(ptr)), PACK(oldsize - size, 1));
    mm_free(NEXT_BLKP(ptr));
    return ptr;
  }

  newptr = mm_malloc(size);

  // if realloc fails, original block is left untouched
  if(!newptr) {
    return 0;
  }

  // copy old data
  if(size < oldsize)
    oldsize = size;
  memcpy(newptr, ptr, oldsize);

  // free old block
  mm_free(ptr);

  return newptr;
}

// insertBlock - inserts a block at front of free list
static void insertBlock(void *bp) {
  NEXT_FREE(bp) = free_listp; // sets next ptr to start of free list
  PREV_FREE(free_listp) = bp; // sets current's prev to new block
  PREV_FREE(bp) = NULL; // sets prev pointer to NULL
  free_listp = bp; // sets start of free list as new block
}

// removeBlock - removes block from free list
static void removeBlock(void *bp) {
  // if theres a previous block, set pointer to next block
  if(PREV_FREE(bp))
    NEXT_FREE(PREV_FREE(bp)) = NEXT_FREE(bp);
  //if not, set block's previous pointer to the next block
  else
    free_listp = NEXT_FREE(bp);

    PREV_FREE(NEXT_FREE(bp)) = PREV_FREE(bp);
}

#ifdef DEBUG
// mm_check - consistency checker
int mm_check(int verbose) {
  // Is every block in the free list marked as free?
  void *next;
  for(next = free_listp; GET_ALLOC(HDRP(next)) == 0; next = NEXT_FREE(next)) {
    if(GET_ALLOC(HDRP(next))) {
      printf("Consistency error: block %p in free list but marked allocated!", next);
      return 1;
    }
  }

  // Are there any contiguous free blocks that escaped coalescing?
  for(next = free_listp; GET_ALLOC(HDRP(next)) == 0; next = NEXT_FREE(next)) {
    char *prev = PREV_FREE(HDRP(next));
    if(prev != NULL && HDRP(next) - FTRP(prev) == DSIZE) {
      printf("Consistency error: block %p missed coalescing!", next);
      return 1;
    }
  }

  // Do the pointers in the free list point to valid free blocks?
  for(next = free_listp; GET_ALLOC(HDRP(next)) == 0; next = NEXT_FREE(next)) {
    if(next < mem_heap_lo() || next > mem_heap_hi()) {
      printf("Consistency error: free block %p invalid", next);
      return 1;
    }
  }

  // Do the pointers in a heap block point to a valid heap address?
  for (next = heap_listp; NEXT_BLKP(next) != NULL; next = NEXT_BLKP(next)) {
    if(next < mem_heap_lo() || next > mem_heap_hi()) {
      printf("Consistency error: block %p outside designated heap space", next);
      return 1;
    }
  }

  return 0;
}

#endif