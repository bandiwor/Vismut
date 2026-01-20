#include "errors.h"

#include <stdlib.h>


const wchar_t *GetVismutErrorString(const VismutError err) {
    switch (err) {
        case VISMUT_ERROR_ALLOC:
            return L"Memory allocation in heap error";
        case VISMUT_ERROR_ENCODING:
            return L"Encoding error";
        case VISMUT_ERROR_IO:
            return L"IO error";
        case VISMUT_ERROR_BUFFER_OVERFLOW:
            return L"Buffer overflow error";
        case VISMUT_ERROR_UNKNOWN_NUMBER_FORMAT:
            return L"Number format error";
        case VISMUT_ERROR_UNKNOWN_CHAR:
            return L"Unknown char error";
        case VISMUT_ERROR_NUMBER_OVERFLOW:
            return L"Number overflow error";
        case VISMUT_ERROR_NUMBER_PARSE:
            return L"Number parsing error";
        case VISMUT_ERROR_UNEXPECTED_TOKEN:
            return L"Unexpected token";
        case VISMUT_ERROR_SYMBOL_ALREADY_DEFINED:
            return L"Symbol is already defined";
        case VISMUT_ERROR_SYMBOL_NOT_DEFINED:
            return L"Symbol is not defined";
        case VISMUT_ERROR_UNSUPPORTED_OPERATION:
            return L"Unsupported operation";
        case VISMUT_ERROR_TYPE_IS_INCOMPATIBLE:
            return L"Type is incompatible";
        case VISMUT_ERROR_CAST_IS_NOT_ALLOWED:
            return L"Cast is not allowed";
        case VISMUT_ERROR_UNKNOWN_TYPE:
            return L"Unknown type";
        default:
            return L"Unknown error";
    }
}

const wchar_t *GetErrorString(const errno_t err) {
    if (err == 0) {
        return L"No error.";
    }

    if (err >= VISMUT_ERRORS_START) {
        return GetVismutErrorString((VismutError) err);
    }

    return _wcserror(err);
}
