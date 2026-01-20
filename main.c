#include <math.h>
#include <stdlib.h>
#include <windows.h>

#include "Vismut/core/ansi_colors.h"
#include "Vismut/core/ast/ast_analyze.h"
#include "Vismut/core/ast/ast_optimize.h"
#include "Vismut/core/ast/ast_parse.h"
#include "Vismut/core/errors/errors.h"
#include "Vismut/core/gen/codegen.h"
#include "Vismut/core/gen/run.h"
#include "Vismut/core/tokenizer/tokenizer.h"
#include "Vismut/io/reader/reader.h"

#include "Vismut/core/memory/arena.h"


int main(int argc, const char **argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    win_ansi_init();

    errno_t err;

    const char *filename = argv[1] == NULL ? "..\\code.txt" : argv[1];
    const size_t filename_len = strlen(filename);

    char c_filename[filename_len + 3];
    memcpy(c_filename, filename, strlen(filename));
    c_filename[filename_len] = '.';
    c_filename[filename_len + 1] = 'c';
    c_filename[filename_len + 2] = '\0';
    char exe_filename[filename_len + 5];
    memcpy(exe_filename, filename, strlen(filename));
    exe_filename[filename_len] = '.';
    exe_filename[filename_len + 1] = 'e';
    exe_filename[filename_len + 2] = 'x';
    exe_filename[filename_len + 3] = 'e';
    exe_filename[filename_len + 4] = '\0';

    StringView text;
    if ((err = Reader_ReadFile(filename, &text)) != 0) {
        printf("%s\n", GetErrorString(err));
        return EXIT_FAILURE;
    }

    Arena *arena = Arena_Create(ARENA_BLOCK_SIZE_DEFAULT);

    Tokenizer tokenizer = Tokenizer_Create(text.data, text.length, (uint8_t *) filename, arena);
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

    FILE *file = fopen(c_filename, "wb");
    if (file == NULL) {
        return EXIT_FAILURE;
    }
    CodeGen_GenerateFromAST(CodeGen_CreateContext(file), ast_parser.module_node);
    fclose(file);
    Arena_Destroy(arena);
    free(text.data);

    Run(c_filename, exe_filename);

    return 0;
}
