//
// Created by kir on 19.12.2025.
//

#ifndef VISMUT_SCOPE_H
#define VISMUT_SCOPE_H
#include "../types.h"
#include "value.h"
#include "../memory/arena.h"

#define SYMBOL_FLAG_INITIALIZED           (1 << 0)
#define SYMBOL_FLAG_CONST                 (1 << 1)
#define SYMBOL_FLAG_CONST_EVAL            (1 << 2)
#define SYMBOL_FLAG_USED                  (1 << 3)
#define SYMBOL_FLAG_USED_MORE_ONCE        (1 << 4)

typedef struct tag_Symbol {
    struct tag_Symbol *next;
    const uint8_t *name;
    VValue value;
    uint32_t hash;
    uint32_t flags;
} Symbol;

typedef struct {
    Symbol *head;
} Slot;

typedef struct tag_Scope {
    struct tag_Scope *parent;
    Arena *allocator;
    Slot *slots;
    size_t capacity;
    size_t size;
    uint8_t depth;
} Scope;

Scope *Scope_Allocate(Arena *allocator, Scope *parent);

errno_t Scope_Declare(Scope *scope, const uint8_t *name, VValueType type, uint32_t flags);

errno_t Scope_RemoveUnused(Scope *scope);

Symbol *Scope_Resolve(const Scope *scope, const uint8_t *name);

errno_t Scope_AssignConstantEvaluated(const Scope *scope, const uint8_t *name, VValue value);

void Scope_MarkInitialized(const Scope *scope, const uint8_t *name);

void Scope_MarkUsed(const Scope *scope, const uint8_t *name);

#endif //VISMUT_SCOPE_H
