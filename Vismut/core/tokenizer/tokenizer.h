//
// Created by kir on 09.10.2025.
//

#ifndef VISMUT_TOKENIZER_H
#define VISMUT_TOKENIZER_H
#include "token.h"
#include "../memory/arena.h"

typedef struct {
    const uint8_t *source_filename;
    const uint8_t *start;
    const uint8_t *cursor;
    const uint8_t *limit;
    const uint8_t *token_start;
    Arena *arena;
} Tokenizer;

Tokenizer Tokenizer_Create(const uint8_t *source, size_t source_length, const uint8_t *source_filename, Arena *);

void Tokenizer_Reset(Tokenizer *);

errno_t Tokenizer_Next(Tokenizer *, VToken *);

#endif //VISMUT_TOKENIZER_H
