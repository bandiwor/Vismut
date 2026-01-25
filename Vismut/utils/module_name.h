//
// Created by kir on 24.01.2026.
//

#ifndef VISMUT_MODULE_NAME_H
#define VISMUT_MODULE_NAME_H
#include <stdint.h>
#include "../core/memory/arena.h"

uint8_t *CreateModuleName(Arena *arena, const uint8_t *filename, int length);

#endif //VISMUT_MODULE_NAME_H
