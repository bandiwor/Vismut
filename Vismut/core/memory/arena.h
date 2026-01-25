//
// Created by kir on 14.12.2025.
//

#ifndef VISMUT_ARENA_H
#define VISMUT_ARENA_H
#include <stdint.h>

#define ARENA_BLOCK_SIZE_DEFAULT 4096

typedef struct tag_ArenaBlock  {
    void *memory;
    size_t size;
    size_t used;
    struct tag_ArenaBlock *next;
} ArenaBlock;

typedef struct {
    ArenaBlock *first;
    ArenaBlock *current;
    size_t block_size;
} Arena;

ArenaBlock *ArenaBlock_Create(size_t);

void ArenaBlock_Destroy(ArenaBlock *);

Arena *Arena_Create(size_t block_size);

void Arena_Destroy(Arena *);

void *Arena_AllocateAligned(Arena *arena, size_t size, size_t align);

#define Arena_Type(arena, type) Arena_AllocateAligned(arena, sizeof(type), __alignof(type))
#define Arena_Array(arena, type, count) Arena_AllocateAligned(arena, sizeof(type) * (count), __alignof(type))

#endif //VISMUT_ARENA_H
