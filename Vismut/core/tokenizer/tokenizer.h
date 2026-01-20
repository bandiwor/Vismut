//
// Created by kir on 09.10.2025.
//

#ifndef VISMUT_TOKENIZER_H
#define VISMUT_TOKENIZER_H
#include "token.h"
#include "../memory/arena.h"

typedef struct {
    wchar_t *source_filename;
    const wchar_t *source;
    size_t source_length;
    Arena *arena;
    size_t start_token_position;
    size_t current_position;
    wchar_t current_char;
} Tokenizer;

Tokenizer Tokenizer_Create(const wchar_t *source, size_t source_length, wchar_t *source_filename, Arena *);

void Tokenizer_Reset(Tokenizer *);

errno_t Tokenizer_Next(Tokenizer *, VToken *);

#endif //VISMUT_TOKENIZER_H
