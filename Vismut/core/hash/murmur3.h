//
// Created by kir on 21.12.2025.
//

#ifndef VISMUT_MURMUR3_H
#define VISMUT_MURMUR3_H
#include "../types.h"
#include <stdint.h>

#define MURMURHASH3_DEFAULT_STR_SEED 0x9747b28c

attribute_pure uint32_t murmurhash3_string(const char *str, uint32_t seed);

attribute_pure uint32_t murmurhash3_wstring(const wchar_t *str, uint32_t seed);

attribute_const uint32_t murmurhash3_int64(int64_t value, uint32_t seed);

attribute_const uint32_t murmurhash3_double(double value, uint32_t seed);

attribute_const uint32_t murmurhash3_combine(uint32_t h1, uint32_t h2);

#endif //VISMUT_MURMUR3_H
