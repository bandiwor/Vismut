//
// Created by kir on 21.01.2026.
//

#ifndef VISMUT_FIND_POSITION_H
#define VISMUT_FIND_POSITION_H
#include <stdint.h>
#include "../core/types.h"

typedef struct в {
    const uint8_t *line_start;
    const uint8_t *line_end;
    size_t line; // 1-based value
    size_t column; // 1-based value
} TextPosition;

attribute_pure
TextPosition FindPosition(const uint8_t *source, const uint8_t* source_end, const uint8_t *ptr);

#endif //VISMUT_FIND_POSITION_H
