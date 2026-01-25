#include "errors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ansi_colors.h"
#include "../../utils/find_position.h"


const char *GetVismutErrorString(const VismutError err) {
    switch (err) {
        case VISMUT_ERROR_ALLOC:
            return "Memory allocation in heap error";
        case VISMUT_ERROR_ENCODING:
            return "Encoding error";
        case VISMUT_ERROR_IO:
            return "IO error";
        case VISMUT_ERROR_BUFFER_OVERFLOW:
            return "Buffer overflow error";
        case VISMUT_ERROR_UNKNOWN_NUMBER_FORMAT:
            return "Number format error";
        case VISMUT_ERROR_UNKNOWN_SYMBOL:
            return "Unknown symbol error";
        case VISMUT_ERROR_NUMBER_OVERFLOW:
            return "Number overflow error";
        case VISMUT_ERROR_NUMBER_PARSE:
            return "Number parsing error";
        case VISMUT_ERROR_UNEXPECTED_TOKEN:
            return "Unexpected token";
        case VISMUT_ERROR_SYMBOL_ALREADY_DEFINED:
            return "Symbol is already defined";
        case VISMUT_ERROR_SYMBOL_NOT_DEFINED:
            return "Symbol is not defined";
        case VISMUT_ERROR_UNSUPPORTED_OPERATION:
            return "Unsupported operation";
        case VISMUT_ERROR_TYPE_IS_INCOMPATIBLE:
            return "Type is incompatible";
        case VISMUT_ERROR_CAST_IS_NOT_ALLOWED:
            return "Cast is not allowed";
        case VISMUT_ERROR_UNKNOWN_TYPE:
            return "Unknown type";
        case VISMUT_ERROR_ASSIGN_NOT_TO_VAR:
            return "You try assign value to not variable reference";
        case VISMUT_ERROR_VOID_FOR_EXPRESSION_FUNCTION:
            return "You try to set VOID return type for expression function, try remove return type annotation.";
        case VISMUT_ERROR_OK:
            return "No error";
        case VISMUT_ERROR_UNEXPECTED_SYMBOL:
            return "Unexpected symbol";
        case VISMUT_ERROR_FUNCTION_ALREADY_DEFINED:
            return "Function is already defined";
        case VISMUT_ERROR_FUNCTION_NOT_DEFINED:
            return "Function not defined";
        case VISMUT_ERROR_INVALID_ARGUMENTS_COUNT:
            return "Invalid arguments count passed to function call";
        case VISMUT_ERROR_INVALID_ARGUMENT_TYPE:
            return "VISMUT_ERROR_INVALID_ARGUMENT_TYPE";
        case VISMUT_ERROR_COUNT:
        default:
            return "Unknown error";
    }
}

const char *GetErrorString(const errno_t err) {
    if (err == 0) {
        return "No error.";
    }

    if (err >= VISMUT_ERRORS_START) {
        return GetVismutErrorString((VismutError) err);
    }

    return strerror(err);
}

VismutErrorInfo CreateVismutErrorInfo
(const VismutError error, const uint8_t *module, const uint8_t *source, const int line, const int column,
 const int length) {
    return (VismutErrorInfo){
        .error = error,
        .module = module,
        .source = source,
        .line = line,
        .column = column,
        .length = length,
    };
}

VismutErrorInfo CreateVismutErrorInfoWithDetails
(const VismutError error, const uint8_t *module, const uint8_t *source, const int line, const int column,
 const int length, const VismutErrorDetails details) {
    return (VismutErrorInfo){
        .error = error,
        .module = module,
        .source = source,
        .line = line,
        .column = column,
        .length = length,
        .details = details,
    };
}

static int get_number_width(const unsigned int n) {
    if (n < 10) return 1;
    int w = 1;
    unsigned int number = n / 10;
    while (number) {
        number /= 10;
        ++w;
    }
    return w;
}

void VismutErrorInfo_Print(const VismutErrorInfo info) {
    const TextPosition error_position = FindPosition(info.source, info.source + info.source_length, info.location);

    PRINT_COLOR(ANSI_RED_FG, "An error occurred in the module: \"");
    PRINT_COLOR(ANSI_WHITE_FG, (const char*)info.module);
    PRINTF_COLOR(ANSI_RED_FG, "\". At line %d, column %d!\n", info.line, info.column);
    PRINTF_2COLOR(ANSI_BLACK_FG, ANSI_BRIGHT_WHITE_BG, "%d", info.line);
    printf(" ");
    for (const uint8_t *ptr = error_position.line_start; ptr < error_position.line_end; ++ptr) {
        putchar(*ptr);
    }
    putchar('\n');

    const int padding = 1 + get_number_width((unsigned int) info.line);
    for (int i = 0; i < padding + info.column - 1; ++i) {
        putchar(' ');
    }
    for (int i = 0; i < info.length; ++i) {
        putchar('^');
    }
    printf("-- ");
    PRINT_COLOR(ANSI_BRIGHT_RED_FG, GetVismutErrorString(info.error));
}
