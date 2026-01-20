//
// Created by kir on 09.10.2025.
//

#ifndef VISMUT_TOKENIZER_H
#define VISMUT_TOKENIZER_H
#include "token.h"
#include "../memory/arena.h"

typedef struct {
    const char *source_filename;
    const char *source;
    char *buffer;
    size_t buffer_size;
    size_t source_length;
    Arena *arena;
    size_t start_token_position;
    size_t current_position;
    char current_char;
} Tokenizer;

Tokenizer Tokenizer_Create(const char *source, size_t source_length, const char *source_filename, Arena *);

void Tokenizer_Reset(Tokenizer *);

errno_t Tokenizer_Next(Tokenizer *, VToken *);

#endif //VISMUT_TOKENIZER_H
