//
// Created by kir on 09.10.2025.
//

#ifndef VISMUT_TOKEN_H
#define VISMUT_TOKEN_H
#include <stdint.h>
#include "../types.h"

typedef struct {
    Position position;
    VTokenType type;

    union {
        int64_t i64;
        double f64;
        uint8_t *chars;
    } data;
} VToken;

void Token_Print(const VToken *token);

#endif //VISMUT_TOKEN_H
