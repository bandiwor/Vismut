//
// Created by kir on 23.11.2025.
//

#ifndef VISMUT_TYPE_H
#define VISMUT_TYPE_H
#include <stdint.h>
#include "../types.h"

typedef struct {
    VValueType type;

    union {
        int64_t i64;
        double f64;
        uint8_t *str;
    };
} VValue;


#endif //VISMUT_TYPE_H
