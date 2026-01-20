//
// Created by kir on 11.10.2025.
//

#ifndef VISMUT_CONVERT_H
#define VISMUT_CONVERT_H
#include <stdint.h>

#include "types.h"

int64_t WStrToInt64Bin(const wchar_t *str);

int64_t WStrToInt64Oct(const wchar_t *str);

int64_t WStrToInt64Dec(const wchar_t *str);

int64_t WStrToInt64Hex(const wchar_t *str);

#endif //VISMUT_CONVERT_H
