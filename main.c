#include <math.h>
#include <stdlib.h>
#include <wchar.h>

#include "Vismut/core/ansi_colors.h"
#include "Vismut/core/ast/ast_analyze.h"
#include "Vismut/core/ast/ast_optimize.h"
#include "Vismut/core/ast/ast_parse.h"
#include "Vismut/core/errors/errors.h"
#include "Vismut/core/gen/codegen.h"
#include "Vismut/core/tokenizer/tokenizer.h"
#include "Vismut/io/reader/reader.h"
#include "Vismut/utils/unicode_console.h"

#include "Vismut/core/memory/arena.h"


int main(void) {
    EnableUnicodeConsole();
    win_ansi_init();

    errno_t err;

    FileText text;
    if ((err = Reader_ReadFile(L"..\\code.txt", &text)) != 0) {
        wprintf(L"%ls\n", GetErrorString(err));
        return EXIT_FAILURE;
    }
    wprintf(L"%ls\n", text.data);

    Arena *arena = Arena_Create(ARENA_BLOCK_SIZE_DEFAULT);

    Tokenizer tokenizer = Tokenizer_Create(text.data, text.length, L"code.txt", arena);
    ASTParser ast_parser = ASTParser_Create(&tokenizer);

    if ((err = ASTParser_Parse(&ast_parser)) != VISMUT_ERROR_OK) {
        wprintf(L"%ls\n", GetErrorString(err));
        return err;
    }

    if ((err = ASTModuleTypeAnalyze(arena, ast_parser.module_node)) != VISMUT_ERROR_OK) {
        wprintf(L"%ls\n", GetErrorString(err));
        return err;
    }

    if ((err = ASTOptimize(arena, ast_parser.module_node)) != VISMUT_ERROR_OK) {
        wprintf(L"%ls\n", GetErrorString(err));
        return err;
    }

    ASTNode_Print(ast_parser.module_node);

    FILE *file = _wfopen(L"..\\code.c", L"w");
    if (file == NULL) {
        return EXIT_FAILURE;
    }

    CodeGen_GenerateFromAST(CodeGen_CreateContext(file), ast_parser.module_node);

    fclose(file);

    Arena_Destroy(arena);

    free(text.data);
    return 0;
}
