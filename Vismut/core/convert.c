#include "convert.h"


attribute_pure
int64_t StrToInt64Bin(const uint8_t *str) {
    int64_t result = 0;
    while (*str) {
        result = (result << 1) | (*str++ - '0');
    }
    return result;
}

attribute_pure
int64_t StrToInt64Oct(const uint8_t *str) {
    int64_t result = 0;
    while (*str) {
        result = (result << 3) | (*str++ - '0');
    }
    return result;
}

attribute_pure
int64_t StrToInt64Dec(const uint8_t *str) {
    int64_t result = 0;
    while (*str) {
        result = result * 10 + (*str++ - '0');
    }
    return result;
}

attribute_pure
int64_t StrToInt64Hex(const uint8_t *str) {
    int64_t result = 0;

    while (*str) {
        const uint8_t c = *str++;

        const int value = (c >= '0' && c <= '9')
                              ? (c - '0')
                              : (c >= 'A' && c <= 'F')
                                    ? (c - 'A' + 10)
                                    : (c >= 'a' && c <= 'f')
                                          ? (c - 'a' + 10)
                                          : 0;

        result = (result << 4) | (value & 0xF);
    }

    return result;
}
