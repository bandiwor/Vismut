//
// Created by kir on 14.12.2025.
//

#ifndef VISMUT_AST_PARSE_H
#define VISMUT_AST_PARSE_H
#include "../memory/arena.h"
#include "../tokenizer/tokenizer.h"
#include "ast.h"

typedef struct {
    const char *source;
    size_t source_length;
    Arena *arena;
    Tokenizer *tokenizer;
    VToken current_token;
    ASTNode *module_node;
    Scope *current_scope;
} ASTParser;

ASTParser ASTParser_Create(Tokenizer *tokenizer);

errno_t ASTParser_Parse(ASTParser *);

#endif //VISMUT_AST_PARSE_H
