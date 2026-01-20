//
// Created by kir on 16.12.2025.
//

#ifndef VISMUT_CALLSTACK_H
#define VISMUT_CALLSTACK_H
#include <wchar.h>
#include "../debug.h"

#if DEBUG
#define CALLSTACK_MAX_DEPTH 1024

typedef struct {
    const char *function_name;
    const char *file_name;
    int line_number;
    const wchar_t *additional_info;
} CallStackEntry;

void CallStack_Init(void);

void CallStack_Push(const char *func, const char *file, int line, const wchar_t *info);

void CallStack_Pop(void);

int CallStack_GetDepth(void);

void CallStack_Print(void);

void CallStack_PrintTop(int count);

#define CALLSTACK_ENTER(info) CallStack_Push(__FUNCTIONW__, __FILEW__, __LINE__, (info))
#define CALLSTACK_LEAVE() CallStack_Pop()
#define CALLSTACK_PRINT() CallStack_Print()
#define CALLSTACK_PRINT_TOP(count) CallStack_PrintTop(count)

#define CALLSTACK_TRACE() \
    CallStackEntry __trace_entry = { __FUNCTION__, __FILE__, __LINE__, NULL }; \
    CallStack_Push(__FUNCTION__, __FILE__, __LINE__, NULL); \
    void __attribute__((cleanup(__trace_cleanup))) *__trace_ptr = &__trace_entry

static inline void __trace_cleanup(const void *entry) {
    (void) entry; // Не используется, нужен только для вызова CallStack_Pop
    CallStack_Pop();
}
#else
#define CALLSTACK_ENTER(info) (void)0
#define CALLSTACK_LEAVE() (void)0
#define CALLSTACK_PRINT() (void)0
#define CALLSTACK_PRINT_TOP(count) (void)0
#define CALLSTACK_TRACE() (void)0

#endif

#endif //VISMUT_CALLSTACK_H
