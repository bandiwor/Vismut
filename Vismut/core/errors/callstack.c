#include "callstack.h"

#include <stdio.h>
#include <string.h>


#if DEBUG
static CallStackEntry callstack[CALLSTACK_MAX_DEPTH];
static int callstack_depth = 0;
static int callstack_initialized = 0;

void CallStack_Init(void) {
    if (callstack_initialized) return;

    callstack_depth = 0;
    callstack_initialized = 1;

    for (int i = 0; i < CALLSTACK_MAX_DEPTH; ++i) {
        callstack[i].function_name = NULL;
        callstack[i].file_name = NULL;
        callstack[i].line_number = 0;
        callstack[i].additional_info = NULL;
    }
}

void CallStack_Push(const char *func, const char *file, const int line, const char *info) {
    if (!callstack_initialized) {
        CallStack_Init();
    }

    if (callstack_depth >= CALLSTACK_MAX_DEPTH) {
        fprintf(stderr, "[CallStack_Push (callstack.c)] CallStack overflow! Max depth: %d\n", CALLSTACK_MAX_DEPTH);
        return;
    }

    callstack[callstack_depth].function_name = func;
    callstack[callstack_depth].file_name = file;
    callstack[callstack_depth].line_number = line;
    callstack[callstack_depth].additional_info = info;

    ++callstack_depth;
}

void CallStack_Pop(void) {
    if (callstack_depth <= 0) {
        fprintf(stderr, "[CallStack_Push (callstack.c)] CallStack underflow!\n");
        return;
    }

    callstack_depth--;

    callstack[callstack_depth].function_name = NULL;
    callstack[callstack_depth].file_name = NULL;
    callstack[callstack_depth].line_number = 0;
    callstack[callstack_depth].additional_info = NULL;
}

int CallStack_GetDepth(void) {
    return callstack_depth;
}

void CallStack_Print(void) {
    if (callstack_depth == 0) {
        fprintf(stderr, "Call stack is empty\n");
        return;
    }

    fprintf(stderr, "\n══════════════════════════════════════════════════════\n");
    fprintf(stderr, "Call Stack Trace (most recent call last):\n");
    fprintf(stderr, "══════════════════════════════════════════════════════\n");

    for (int i = 0; i < callstack_depth; i++) {
        fprintf(stderr, "%4d. ", i + 1);

        if (callstack[i].function_name) {
            fprintf(stderr, "%hs", callstack[i].function_name);
        } else {
            fprintf(stderr, "<unknown>");
        }

        fprintf(stderr, " at ");

        if (callstack[i].file_name) {
            // Извлекаем только имя файла из полного пути
            const char *filename = callstack[i].file_name;
            const char *slash = strrchr(filename, '/');
            if (!slash) slash = strrchr(filename, '\\');
            if (slash) filename = slash + 1;

            fprintf(stderr, "%hs:%d", filename, callstack[i].line_number);
        } else {
            fprintf(stderr, "<unknown file>");
        }

        if (callstack[i].additional_info) {
            fprintf(stderr, " [%s]", callstack[i].additional_info);
        }

        fprintf(stderr, "\n");
    }

    fprintf(stderr, "══════════════════════════════════════════════════════\n\n");
}


void CallStack_PrintTop(int count) {
    if (callstack_depth == 0) {
        fprintf(stderr, "Call stack is empty\n");
        return;
    }

    if (count > callstack_depth) {
        count = callstack_depth;
    }

    fprintf(stderr, "\nTop %d frames of call stack:\n", count);
    fprintf(stderr, "────────────────────────────────────────────\n");

    // Выводим только верхние count фреймов
    int start = callstack_depth - count;
    for (int i = start; i < callstack_depth; i++) {
        fprintf(stderr, "%3d. ", i - start + 1);

        if (callstack[i].function_name) {
            fprintf(stderr, "%hs", callstack[i].function_name);
        }

        if (callstack[i].file_name) {
            const char *filename = callstack[i].file_name;
            const char *slash = strrchr(filename, L'/');
            if (!slash) slash = strrchr(filename, L'\\');
            if (slash) filename = slash + 1;

            fprintf(stderr, " (%hs:%d)", filename, callstack[i].line_number);
        }

        if (callstack[i].additional_info) {
            fprintf(stderr, " [%s]", callstack[i].additional_info);
        }

        fprintf(stderr, "\n");
    }

    fprintf(stderr, "────────────────────────────────────────────\n");
}

#endif
