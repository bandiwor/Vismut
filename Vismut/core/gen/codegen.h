//
// Created by kir on 18.01.2026.
//

#ifndef VISMUT_CODEGEN_H
#define VISMUT_CODEGEN_H
#include <stdio.h>
#include "../memory/arena.h"
#include "../ast/ast.h"

typedef struct {
    FILE *output;
} CodeGenContext;

attribute_pure
CodeGenContext CodeGen_CreateContext(FILE *output);

void CodeGen_GenerateFromAST(CodeGenContext ctx, const ASTNode *module);

#endif //VISMUT_CODEGEN_H
