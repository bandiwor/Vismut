//
// Created by kir on 11.10.2025.
//

#ifndef VISMUT_CONVERT_H
#define VISMUT_CONVERT_H
#include <stdint.h>

#include "types.h"

int64_t StrToInt64Bin(const uint8_t *str);

int64_t StrToInt64Oct(const uint8_t *str);

int64_t StrToInt64Dec(const uint8_t *str);

int64_t StrToInt64Hex(const uint8_t *str);

#endif //VISMUT_CONVERT_H
