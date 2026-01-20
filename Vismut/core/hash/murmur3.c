#include "murmur3.h"
#include <string.h>

// Константы MurmurHash3
#define MURMUR3_C1 0xcc9e2d51
#define MURMUR3_C2 0x1b873593
#define MURMUR3_R1 15
#define MURMUR3_R2 13
#define MURMUR3_M  5
#define MURMUR3_N  0xe6546b64


// Основная функция MurmurHash3 32-бит
uint32_t murmurhash3_32(const void *key, const size_t len, const uint32_t seed) {
    const uint8_t *data = (const uint8_t *) key;
    const int n_blocks = (int) (len / 4);

    uint32_t h1 = seed;
    const uint32_t *blocks = (const uint32_t *) (data + n_blocks * 4);

    for (int i = -n_blocks; i; i++) {
        uint32_t k1 = blocks[i];

        k1 *= MURMUR3_C1;
        k1 = (k1 << MURMUR3_R1) | (k1 >> (32 - MURMUR3_R1));
        k1 *= MURMUR3_C2;

        h1 ^= k1;
        h1 = (h1 << MURMUR3_R2) | (h1 >> (32 - MURMUR3_R2));
        h1 = h1 * MURMUR3_M + MURMUR3_N;
    }

    // Обработка оставшихся байт
    const uint8_t *tail = (const uint8_t *) (data + n_blocks * 4);
    uint32_t k1 = 0;

    switch (len & 3) {
        case 3: k1 ^= (uint32_t) tail[2] << 16;
        case 2: k1 ^= (uint32_t) tail[1] << 8;
        case 1: k1 ^= (uint32_t) tail[0];
            k1 *= MURMUR3_C1;
            k1 = (k1 << MURMUR3_R1) | (k1 >> (32 - MURMUR3_R1));
            k1 *= MURMUR3_C2;
            h1 ^= k1;
        default: break;
    }

    // Финальный микс
    h1 ^= (uint32_t) len;
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1;
}

// Оптимизированная версия для C-строк
uint32_t murmurhash3_string(const char *str, const uint32_t seed) {
    if (!str) return 0;
    return murmurhash3_32(str, strlen(str), seed);
}

// Хеширование 64-битного целого
uint32_t murmurhash3_int64(const int64_t value, const uint32_t seed) {
    uint32_t h1 = seed;

    // Обрабатываем как два 32-битных значения
    uint32_t k1 = (uint32_t) (value & 0xFFFFFFFF);
    uint32_t k2 = (uint32_t) (value >> 32);

    // Первая половина
    k1 *= MURMUR3_C1;
    k1 = (k1 << MURMUR3_R1) | (k1 >> (32 - MURMUR3_R1));
    k1 *= MURMUR3_C2;
    h1 ^= k1;
    h1 = (h1 << MURMUR3_R2) | (h1 >> (32 - MURMUR3_R2));
    h1 = h1 * MURMUR3_M + MURMUR3_N;

    // Вторая половина
    k2 *= MURMUR3_C1;
    k2 = (k2 << MURMUR3_R1) | (k2 >> (32 - MURMUR3_R1));
    k2 *= MURMUR3_C2;
    h1 ^= k2;
    h1 = (h1 << MURMUR3_R2) | (h1 >> (32 - MURMUR3_R2));
    h1 = h1 * MURMUR3_M + MURMUR3_N;

    // Финальный микс
    h1 ^= 8; // Длина в байтах
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1;
}

// Хеширование double
uint32_t murmurhash3_double(const double value, const uint32_t seed) {
    union {
        double d;
        uint64_t i;
    } converter;

    converter.d = value;
    return murmurhash3_int64((int64_t) converter.i, seed);
}

// Комбинирование двух хешей
uint32_t murmurhash3_combine(const uint32_t h1, const uint32_t h2) {
    uint32_t h1_ = h1;

    h1_ ^= h2;
    h1_ *= 0x85ebca6b;
    h1_ ^= h1_ >> 13;
    h1_ *= 0xc2b2ae35;
    h1_ ^= h1_ >> 16;
    return h1_;
}
