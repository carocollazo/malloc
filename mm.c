#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "./memlib.h"
#include "./mm.h"
#include "./mminline.h"

// block_t *prol;
// block_t *epil;

// rounds up to the nearest multiple of WORD_SIZE
static inline size_t align(size_t size) {
  return (((size) + (WORD_SIZE - 1)) & ~(WORD_SIZE - 1));
}

/*
 *                             _       _ _
 *     _ __ ___  _ __ ___     (_)_ __ (_) |_
 *    | '_ ` _ \| '_ ` _ \    | | '_ \| | __|
 *    | | | | | | | | | | |   | | | | | | |_
 *    |_| |_| |_|_| |_| |_|___|_|_| |_|_|\__|
 *                       |_____|
 *
 * initializes the dynamic storage allocator (allocate initial heap space)
 * arguments: none
 * returns: 0, if successful
 *         -1, if an error occurs
 */
int mm_init(void) {
  if ((prol = (block_t *)mem_sbrk((int)TAGS_SIZE)) == (block_t *)-1) {
    return -1;
  }
  block_set_size_and_allocated(prol, TAGS_SIZE, 1);

  if ((epil = (block_t *)mem_sbrk((int)TAGS_SIZE)) == (block_t *)-1) {
    return -1;
  }
  block_set_size_and_allocated(epil, TAGS_SIZE, 1);
  
  flist_first = NULL; 
  return 0; 
}

/*     _ __ ___  _ __ ___      _ __ ___   __ _| | | ___   ___
 *    | '_ ` _ \| '_ ` _ \    | '_ ` _ \ / _` | | |/ _ \ / __|
 *    | | | | | | | | | | |   | | | | | | (_| | | | (_) | (__
 *    |_| |_| |_|_| |_| |_|___|_| |_| |_|\__,_|_|_|\___/ \___|
 *                       |_____|
 *
 * allocates a block of memory and returns a pointer to that block's payload
 * arguments: size: the desired payload size for the block
 * returns: a pointer to the newly-allocated block's payload (whose size
 *          is a multiple of ALIGNMENT), or NULL if an error occurred
 */
void *mm_malloc(size_t size) {
  if (size < 1) { 
    return  NULL;
  }
  
  block_t *new_b;
  block_t *free_b = flist_first;
  size_t free_size = block_size(flist_first);

  size = align(size) + TAGS_SIZE;

  if (flist_first == NULL) {
    new_b = epil;
    if (mem_sbrk(size) == (void *)-1) {
      return NULL;
    }
    block_set_size_and_allocated(new_b, size, 1);
    epil = block_next(new_b);
    block_set_size_and_allocated(epil, TAGS_SIZE, 1);
    return new_b->payload;
  }

  if (free_size >= size) {
    pull_free_block(flist_first);
    if (free_size >= size + MINBLOCKSIZE) {
        block_set_size_and_allocated(free_b, size, 1);
        new_b = block_next(free_b);
        block_set_size_and_allocated(new_b, (free_size - size), 0);
        insert_free_block(new_b);
        return free_b->payload;
    }
      block_set_size_and_allocated(free_b, size, 1);
      return free_b->payload;
  }

  free_b = block_flink(free_b);

  while (free_b != flist_first)
  {
    free_size = block_size(free_b);
    if (free_size >= size) {
      pull_free_block(free_b);
      free_size = block_size(free_b);
      if (free_size >= size) {
        pull_free_block(free_b);
        if (free_size >= size + MINBLOCKSIZE) {
          block_set_size_and_allocated(free_b, size, 1);
          new_b = block_next(free_b);
          block_set_size_and_allocated(new_b,(free_size - size), 0);
          insert_free_block(new_b);
          return free_b->payload;
        }
      }
      free_b = block_flink(free_b);
    }
  }

  if (!block_prev_allocated(epil)) {
    pull_free_block(block_prev(epil));
    new_b = block_prev(epil);
    if (mem_sbrk((int)size - (int)block_prev_size(epil)) == (void *)-1) {
      return NULL;
    }
    block_set_size_and_allocated(new_b, size, 1);
    epil = block_next(new_b);
    block_set_size_and_allocated(epil, TAGS_SIZE, 1);
    return new_b->payload;
  }

  new_b = epil;
  if (mem_sbrk((int)size) == (void *)-1) {
    return NULL;
  }
  block_set_size_and_allocated(new_b, size, 1);
  epil = block_next(new_b);
  block_set_size_and_allocated(epil, TAGS_SIZE, 1);
  return new_b->payload;
}

/*                              __
 *     _ __ ___  _ __ ___      / _|_ __ ___  ___
 *    | '_ ` _ \| '_ ` _ \    | |_| '__/ _ \/ _ \
 *    | | | | | | | | | | |   |  _| | |  __/  __/
 *    |_| |_| |_|_| |_| |_|___|_| |_|  \___|\___|
 *                       |_____|
 *
 * frees a block of memory, enabling it to be reused later
 * arguments: ptr: pointer to the block's payload
 * returns: nothing
 */
void mm_free(void *ptr) {
  if (ptr == NULL) {
    return;
  }

  block_t *b = payload_to_block(ptr);
  block_t *prev;
  block_t *next;
  size_t b_size;
  size_t prev_size;
  size_t next_size;
  
  block_set_allocated(b, 0);

  prev = block_prev(b);
  next = block_next(b);
  prev_size = block_size(prev);
  next_size = block_size(next);
  b_size = block_size(b);

  if (!block_allocated(prev) && block_allocated(next)) {
    pull_free_block(prev);
    block_set_size_and_allocated(prev, prev_size + b_size, 0);
    insert_free_block(prev);
  } else if (!block_allocated(next) && block_allocated(prev)) {
    pull_free_block(next);
    block_set_size_and_allocated(b, b_size + next_size, 0);
    insert_free_block(b);
  } else if (!block_allocated(prev) && !block_allocated(next)) {
    pull_free_block(prev);
    pull_free_block(next);
    block_set_size_and_allocated(prev, prev_size + b_size + next_size, 0);
    insert_free_block(prev);
  } else {
    block_set_size_and_allocated(b, b_size, 0);
    insert_free_block(b);
  }
}

/*
 *                                            _ _
 *     _ __ ___  _ __ ___      _ __ ___  __ _| | | ___   ___
 *    | '_ ` _ \| '_ ` _ \    | '__/ _ \/ _` | | |/ _ \ / __|
 *    | | | | | | | | | | |   | | |  __/ (_| | | | (_) | (__
 *    |_| |_| |_|_| |_| |_|___|_|  \___|\__,_|_|_|\___/ \___|
 *                       |_____|
 *
 * reallocates a memory block to update it with a new given size
 * arguments: ptr: a pointer to the memory block's payload
 *            size: the desired new payload size
 * returns: a pointer to the new memory block's payload
 */
void *mm_realloc(void *ptr, size_t size) {
  if (size == 0) {
    mm_free(ptr);
    return NULL;
  }
  
  size = align(size) + TAGS_SIZE;
  if (ptr == NULL) {
    return mm_malloc(size); 
  }
  
  block_t *curr_b = payload_to_block(ptr);
  size_t curr_b_size = block_size(curr_b);

  if (curr_b_size == size) {
    return ptr;
  } 
  
  if (curr_b_size > size) {
    int size_diff = curr_b_size - size;
    if (size_diff >= (int)MINBLOCKSIZE) 
    {
      block_set_size_and_allocated(curr_b, size, 1);
      block_set_size_and_allocated(block_next(curr_b), size_diff, 0);
      insert_free_block(block_next(curr_b));
    }
    return ptr;
  }
  char new_alloc[size - TAGS_SIZE];
  memcpy(new_alloc, ptr, size - TAGS_SIZE);
  mm_free(ptr);

  ptr = (void *)mm_malloc(size);
  memcpy(ptr, new_alloc, size - TAGS_SIZE);
  return ptr;
}