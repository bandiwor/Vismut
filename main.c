#include <math.h>
#include <stdlib.h>
#include <wchar.h>
#include <windows.h>

#include "Vismut/core/ansi_colors.h"
#include "Vismut/core/ast/ast_analyze.h"
#include "Vismut/core/ast/ast_optimize.h"
#include "Vismut/core/ast/ast_parse.h"
#include "Vismut/core/errors/errors.h"
#include "Vismut/core/gen/codegen.h"
#include "Vismut/core/tokenizer/tokenizer.h"
#include "Vismut/io/reader/reader.h"

#include "Vismut/core/memory/arena.h"


int main(void) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    win_ansi_init();

    errno_t err;

    StringView text;
    if ((err = Reader_ReadFile("..\\code.txt", &text)) != 0) {
        printf("%s\n", GetErrorString(err));
        return EXIT_FAILURE;
    }

    Arena *arena = Arena_Create(ARENA_BLOCK_SIZE_DEFAULT);

    Tokenizer tokenizer = Tokenizer_Create(text.data, text.length, "code.txt", arena);
    ASTParser ast_parser = ASTParser_Create(&tokenizer);

    if ((err = ASTParser_Parse(&ast_parser)) != VISMUT_ERROR_OK) {
        printf("%s\n", GetErrorString(err));
        return err;
    }

    if ((err = ASTModuleTypeAnalyze(arena, ast_parser.module_node)) != VISMUT_ERROR_OK) {
        printf("%s\n", GetErrorString(err));
        return err;
    }

    if ((err = ASTOptimize(arena, ast_parser.module_node)) != VISMUT_ERROR_OK) {
        printf("%s\n", GetErrorString(err));
        return err;
    }

    ASTNode_Print(ast_parser.module_node);

    FILE *file = fopen("..\\code.c", "wb");
    if (file == NULL) {
        return EXIT_FAILURE;
    }
    CodeGen_GenerateFromAST(CodeGen_CreateContext(file), ast_parser.module_node);
    fclose(file);

    Arena_Destroy(arena);
    free(text.data);
    return 0;
}
