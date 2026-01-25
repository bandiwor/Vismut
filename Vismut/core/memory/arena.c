#include "arena.h"
#include "../errors/errors.h"

#include <stdlib.h>
#include <stdio.h>

#include "../types.h"

#define ALIGN_FORWARD(ptr, align) (((ptr) + ((align) - 1)) & ~((align) - 1))

#ifndef ARENA_BLOCK_SIZE
#define ARENA_BLOCK_SIZE (64 * 1024)
#endif

#ifndef ARENA_ALIGNMENT
#define ARENA_ALIGNMENT 32
#endif

#ifdef _WIN32
#include <malloc.h>
#define ALIGNED_ALLOC(alignment, size) _aligned_malloc(size, alignment)
#define ALIGNED_FREE(ptr) _aligned_free(ptr)
#else
#include <malloc.h>
#define ALIGNED_ALLOC(alignment, size) aligned_alloc(alignment, size)
#define ALIGNED_FREE(ptr) free(ptr)
#endif

ArenaBlock *ArenaBlock_Create(const size_t size) {
    ArenaBlock *arena_block = calloc(1, sizeof(ArenaBlock));
    if (arena_block == NULL) {
        exit(VISMUT_ERROR_ALLOC);
    }
    void *memory_block = ALIGNED_ALLOC(ARENA_ALIGNMENT, size);
    if (memory_block == NULL) {
        free(arena_block);
        exit(VISMUT_ERROR_ALLOC);
    }

    arena_block->memory = memory_block;
    arena_block->size = size;

    return arena_block;
}

void ArenaBlock_Destroy(ArenaBlock *block) {
    DEBUG_ASSERT(block != NULL);

    if (block->memory != NULL) {
        ALIGNED_FREE(block->memory);
    }

    free(block);
}


Arena *Arena_Create(const size_t block_size) {
    DEBUG_ASSERT(block_size > 0);

    Arena *arena = calloc(1, sizeof(Arena));
    if (arena == NULL) {
        exit(VISMUT_ERROR_ALLOC);
    }

    ArenaBlock *first_block = ArenaBlock_Create(block_size);

    arena->first = first_block;
    arena->current = first_block;
    arena->block_size = block_size;

    return arena;
}

void Arena_Destroy(Arena *arena) {
    DEBUG_ASSERT(arena != NULL);

    ArenaBlock *current_block = arena->first;
    while (current_block != NULL) {
        ArenaBlock *next_block = current_block->next;
        ArenaBlock_Destroy(current_block);
        current_block = next_block;
    }
    free(arena);
}

void *Arena_AllocateAligned(Arena *arena, const size_t size, const size_t align) {
    DEBUG_ASSERT(arena != NULL);

    ArenaBlock *block = arena->current;
    size_t offset = ALIGN_FORWARD(block->used, align);

    if (offset + size > block->size) {
        ArenaBlock *new_block = ArenaBlock_Create(arena->block_size);
        new_block->next = block;
        arena->current = new_block;
        offset = 0;
    }

    void *ptr = (uint8_t *) (block->memory) + offset;
    block->used = offset + size;

    return ptr;
}
