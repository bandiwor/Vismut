#include "convert.h"


attribute_pure
int64_t WStrToInt64Bin(const wchar_t *str) {
    int64_t result = 0;
    while (*str) {
        result = (result << 1) | (*str++ - L'0');
    }
    return result;
}

attribute_pure
int64_t WStrToInt64Oct(const wchar_t *str) {
    int64_t result = 0;
    while (*str) {
        result = (result << 3) | (*str++ - L'0');
    }
    return result;
}

attribute_pure
int64_t WStrToInt64Dec(const wchar_t *str) {
    int64_t result = 0;
    while (*str) {
        result = result * 10 + (*str++ - L'0');
    }
    return result;
}

attribute_pure
int64_t WStrToInt64Hex(const wchar_t *str) {
    int64_t result = 0;

    while (*str) {
        const wchar_t c = *str++;

        const int value = (c >= L'0' && c <= L'9')   ? (c - L'0')
                          : (c >= L'A' && c <= L'F') ? (c - L'A' + 10)
                          : (c >= L'a' && c <= L'f') ? (c - L'a' + 10)
                                                     : 0;

        result = (result << 4) | (value & 0xF);
    }

    return result;
}
