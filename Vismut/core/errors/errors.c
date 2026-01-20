#include "errors.h"

#include <stdlib.h>
#include <string.h>


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
        case VISMUT_ERROR_UNKNOWN_CHAR:
            return "Unknown char error";
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
        case VISMUT_ERROR_OK:
            return "No error";
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
