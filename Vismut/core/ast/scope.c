#include "scope.h"

#include "../errors/errors.h"
#include "../hash/murmur3.h"

#define SCOPE_INITIAL_CAPACITY 4

static void symbol_set_flag(Symbol *sym, const uint32_t flag) {
    DEBUG_ASSERT(sym != NULL);
    sym->flags |= flag;
}

// static void symbol_clear_flag(Symbol *sym, const uint32_t flag) {
//     DEBUG_ASSERT(sym != NULL);
//     sym->flags &= ~flag;
// }

static bool symbol_has_flag(const Symbol *sym, const uint32_t flag) {
    DEBUG_ASSERT(sym != NULL);
    return sym->flags & flag;
}

Scope *Scope_Allocate(Arena *allocator, Scope *parent) {
    Scope *scope = Arena_Type(allocator, Scope);
    scope->allocator = allocator;
    scope->capacity = SCOPE_INITIAL_CAPACITY;
    scope->slots = Arena_Array(allocator, Slot, scope->capacity);
    for (size_t i = 0; i < scope->capacity; ++i) {
        scope->slots[i].head = NULL;
    }

    scope->parent = parent;
    scope->depth = parent ? parent->depth + 1 : 0;
    scope->size = 0;

    return scope;
}

attribute_pure
static size_t slot_index(const Scope *scope, const uint32_t hash) {
    return hash % scope->capacity;
}

static void rehash(Scope *scope, const size_t new_capacity) {
    const Slot *old_slots = scope->slots;
    const size_t old_capacity = scope->capacity;

    scope->slots = Arena_Array(scope->allocator, Slot, new_capacity);
    for (size_t i = 0; i < scope->capacity; ++i) {
        scope->slots[i].head = NULL;
    }
    scope->capacity = new_capacity;
    scope->size = 0;

    for (size_t i = 0; i < old_capacity; ++i) {
        Symbol *sym = old_slots[i].head;

        while (sym) {
            Symbol *next = sym->next;

            const size_t index = slot_index(scope, sym->hash);
            sym->next = scope->slots[index].head;
            scope->slots[index].head = sym;

            ++scope->size;
            sym = next;
        }
    }
}


static Symbol *create_symbol(
    Arena *allocator,
    const wchar_t *name,
    const VValueType type,
    const uint32_t flags,
    const uint32_t hash
) {
    Symbol *sym = Arena_Type(allocator, Symbol);
    *sym = (Symbol){
        .next = NULL,
        .name = name,
        .value = {
            .type = type,
        },
        .hash = hash,
        .flags = flags,
    };
    return sym;
}


errno_t Scope_Declare(Scope *scope, const wchar_t *name,
                      const VValueType type, const uint32_t flags) {
    DEBUG_ASSERT(scope);
    DEBUG_ASSERT(name);

    const uint32_t hash = murmurhash3_wstring(name, MURMURHASH3_DEFAULT_STR_SEED);
    size_t index = slot_index(scope, hash);

    for (const Symbol *sym = scope->slots[index].head; sym; sym = sym->next) {
        if (sym->hash == hash && wcscmp(sym->name, name) == 0) {
            return VISMUT_ERROR_SYMBOL_ALREADY_DEFINED;
        }
    }

    if (scope->size >= scope->capacity * 3 / 4) {
        rehash(scope, scope->capacity * 2);
        index = slot_index(scope, hash);
    }

    Symbol *sym = create_symbol(scope->allocator, name, type, flags, hash);

    sym->next = scope->slots[index].head;
    scope->slots[index].head = sym;

    ++scope->size;
    return VISMUT_ERROR_OK;
}


errno_t Scope_RemoveUnused(Scope *scope) {
    DEBUG_ASSERT(scope);

    for (size_t i = 0; i < scope->capacity; ++i) {
        Symbol **pp = &scope->slots[i].head;

        while (*pp) {
            Symbol *sym = *pp;

            if (!symbol_has_flag(sym, SYMBOL_FLAG_USED)) {
                *pp = sym->next;
                sym->next = NULL;
                --scope->size;
            } else {
                pp = &sym->next;
            }
        }
    }

    return VISMUT_ERROR_OK;
}


Symbol *Scope_Resolve(const Scope *scope, const wchar_t *name) {
    DEBUG_ASSERT(scope);
    DEBUG_ASSERT(name);

    const uint32_t hash = murmurhash3_wstring(name, MURMURHASH3_DEFAULT_STR_SEED);

    for (const Scope *cur = scope; cur; cur = cur->parent) {
        const size_t index = slot_index(cur, hash);

        for (Symbol *sym = cur->slots[index].head; sym; sym = sym->next) {
            if (sym->hash == hash && wcscmp(sym->name, name) == 0) {
                return sym;
            }
        }
    }

    return NULL;
}


errno_t Scope_AssignConstantEvaluated(const Scope *scope, const wchar_t *name, const VValue value) {
    DEBUG_ASSERT(scope != NULL);
    DEBUG_ASSERT(name != NULL);

    Symbol *sym = Scope_Resolve(scope, name);
    if (sym == NULL) {
        return VISMUT_ERROR_SYMBOL_NOT_DEFINED;
    }

    if (sym->value.type != value.type) {
        return VISMUT_ERROR_TYPE_IS_INCOMPATIBLE;
    }

    sym->value = value;
    symbol_set_flag(sym, SYMBOL_FLAG_INITIALIZED | SYMBOL_FLAG_CONST_EVAL);

    return true;
}

void Scope_MarkInitialized(const Scope *scope, const wchar_t *name) {
    DEBUG_ASSERT(scope != NULL);
    DEBUG_ASSERT(name != NULL);

    Symbol *sym = Scope_Resolve(scope, name);
    if (sym != NULL) {
        symbol_set_flag(sym, SYMBOL_FLAG_INITIALIZED);
    }
}

void Scope_MarkUsed(const Scope *scope, const wchar_t *name) {
    DEBUG_ASSERT(scope != NULL);
    DEBUG_ASSERT(name != NULL);

    Symbol *sym = Scope_Resolve(scope, name);
    if (sym != NULL) {
        if (!symbol_has_flag(sym, SYMBOL_FLAG_USED)) {
            symbol_set_flag(sym, SYMBOL_FLAG_USED);
        } else if (!symbol_has_flag(sym, SYMBOL_FLAG_USED_MORE_ONCE)) {
            symbol_set_flag(sym, SYMBOL_FLAG_USED_MORE_ONCE);
        }
    }
}
