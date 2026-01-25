//
// Created by kir on 01.10.2025.
//

#ifndef VISMUT_ERRORS_H
#define VISMUT_ERRORS_H
#include <stdint.h>
#include "../tokenizer/token.h"

#define VISMUT_ERRORS_START 0xffff

typedef enum {
    VISMUT_ERROR_OK = 0,
    VISMUT_ERROR_ALLOC = VISMUT_ERRORS_START + 1,
    VISMUT_ERROR_ENCODING,
    VISMUT_ERROR_IO,
    VISMUT_ERROR_BUFFER_OVERFLOW,
    VISMUT_ERROR_UNKNOWN_NUMBER_FORMAT,
    VISMUT_ERROR_UNKNOWN_SYMBOL,
    VISMUT_ERROR_NUMBER_OVERFLOW,
    VISMUT_ERROR_NUMBER_PARSE,
    VISMUT_ERROR_UNEXPECTED_SYMBOL,
    VISMUT_ERROR_UNEXPECTED_TOKEN,
    VISMUT_ERROR_SYMBOL_ALREADY_DEFINED,
    VISMUT_ERROR_SYMBOL_NOT_DEFINED,
    VISMUT_ERROR_FUNCTION_ALREADY_DEFINED,
    VISMUT_ERROR_FUNCTION_NOT_DEFINED,
    VISMUT_ERROR_UNSUPPORTED_OPERATION,
    VISMUT_ERROR_TYPE_IS_INCOMPATIBLE,
    VISMUT_ERROR_CAST_IS_NOT_ALLOWED,
    VISMUT_ERROR_ASSIGN_NOT_TO_VAR,
    VISMUT_ERROR_VOID_FOR_EXPRESSION_FUNCTION,
    VISMUT_ERROR_INVALID_ARGUMENTS_COUNT,
    VISMUT_ERROR_INVALID_ARGUMENT_TYPE,
    VISMUT_ERROR_UNKNOWN_TYPE,
    VISMUT_ERROR_COUNT
} VismutError;

typedef union {
    struct {
        uint8_t caught;
    } unexpected_symbol;

    struct {
        uint8_t caught;
    } unknown_symbol;

    struct {
        VToken caught;
    } unexpected_token;
} VismutErrorDetails;

typedef struct {
    VismutError error;
    const uint8_t *module;
    const uint8_t *source;
    size_t source_length;
    const uint8_t *location;
    int line; // Line of error. -1 for unknown
    int column; // Column of error. -1 for unknown
    int length; // Length of error. -1 for unknown
    VismutErrorDetails details;
} VismutErrorInfo;

void VismutErrorInfo_Print(VismutErrorInfo);

VismutErrorInfo
CreateVismutErrorInfo(VismutError error, const uint8_t *module, const uint8_t *source, int line, int column,
                      int length);

VismutErrorInfo
CreateVismutErrorInfoWithDetails(VismutError error, const uint8_t *module, const uint8_t *source, int line, int column,
                                 int length, VismutErrorDetails details);


const char *GetErrorString(errno_t);

#endif //VISMUT_ERRORS_H
