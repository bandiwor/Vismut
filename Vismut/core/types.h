//
// Created by kir on 01.10.2025.
//

#ifndef VISMUT_TYPES_H
#define VISMUT_TYPES_H
#include <wchar.h>
#include <stdio.h>
#include "types_maps.h"
#include "errors/callstack.h"
#include "debug.h"

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define attribute_hot __attribute__((hot))
#define attribute_cold __attribute__((cold))
#define attribute_pure __attribute__((pure))
#define attribute_const  __attribute__((const))

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define S1(x) #x
#define S2(x) S1(x)
#define LOCATION __FILE__ " : " S2(__LINE__)

#if DEBUG
#define DEBUG_ASSERT(CONDITION) do {  \
    if (!(CONDITION)) {                          \
        fprintf_s(stderr, "Assertion Failed!\nAt: " LOCATION "\n--> Function: %s\n--> Assertion: \"%s\"\n", __FUNCTION__, STRINGIFY(CONDITION)); \
        CALLSTACK_PRINT();                       \
        exit(1);                                 \
    }                                            \
} while (0)
#define DEBUG_LOG_ASSERT(CONDITION, TEXT) do {  \
    if (!(CONDITION)) {                          \
        fprintf_s(stderr, "Assertion Failed!\nAt: " LOCATION "\n--> Function: %s\n--> Assertion: \"%s\"\n", __FUNCTION__, STRINGIFY(CONDITION)); \
        fprintf_s(stderr, "--> Details: " TEXT);                   \
        CALLSTACK_PRINT();                       \
        exit(1);                                 \
    }                                            \
} while (0)
#define DEBUG_LOG_ASSERT_WITH_ARGS(CONDITION, TEXT, ...) do {  \
    if (!(CONDITION)) {                          \
        fprintf_s(stderr, "Assertion Failed!\nAt: " LOCATION "\n--> Function: %s\n--> Assertion: \"%s\"\n", __FUNCTION__, STRINGIFY(CONDITION)); \
        fprintf_s(stderr, "--> Details: " TEXT, __VA_ARGS__);                   \
        CALLSTACK_PRINT();                       \
        exit(1);                                 \
    }                                            \
} while (0)
#else
#define DEBUG_ASSERT(...) do {} while (0)
#define DEBUG_ASSERT_WITH_ARGS(...) do {} while (0)
#define DEBUG_LOG_ASSERT(...) do {} while (0)
#define DEBUG_LOG_ASSERT_WITH_ARGS(...) do {} while (0)
#endif

typedef enum {
#define X(name, _) name,
    TOKENS_MAP(X)
#undef X
    TOKEN_COUNT
} VTokenType;

attribute_const const char *VTokenType_String(VTokenType);

typedef enum {
#define X(name, _) name,
    AST_BINARY_MAP(X)
#undef X
    AST_BINARY_COUNT
} ASTBinaryType;

attribute_const const char *ASTBinaryType_String(ASTBinaryType);

typedef enum {
#define X(name, _) name,
    AST_UNARY_MAP(X)
#undef X
    AST_UNARY_COUNT
} ASTUnaryType;

attribute_const const char *ASTUnaryType_String(ASTUnaryType);

typedef enum {
#define X(name, _) name,
    AST_NODES_MAP(X)
#undef X
    AST_COUNT
} ASTNodeType;

attribute_const const char *ASTNodeType_String(ASTNodeType);

typedef enum {
#define X(name, _) name,
    VALUE_TYPE_MAP(X)
#undef X
    VALUE_TYPES_COUNT
} VValueType;


attribute_const const char *VValueType_String(VValueType);

typedef struct {
    char *data;
    size_t length;
} StringView;

typedef struct {
    size_t offset;
    size_t length;
} Position;

attribute_const static Position Position_Join(const Position from, const Position to) {
    return (Position){
        from.offset,
        to.offset + to.length - from.offset,
    };
}

#define START_BLOCK_WRAPPER do {
#define END_BLOCK_WRAPPER } while(0)

#define RISKY_EXPRESSION_SAFE(expression, err_var) \
    START_BLOCK_WRAPPER \
        if (((err_var) = (expression)) != VISMUT_ERROR_OK) { \
            return err_var; \
        } \
    END_BLOCK_WRAPPER


#endif //VISMUT_TYPES_H
