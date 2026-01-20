//
// Created by kir on 26.12.2025.
//

#ifndef VISMUT_AST_OPTIMIZE_H
#define VISMUT_AST_OPTIMIZE_H
#include "../types.h"
#include "ast.h"

errno_t ASTOptimize(Arena *arena, ASTNode *node);

#endif //VISMUT_AST_OPTIMIZE_H