//
// Created by kir on 01.10.2025.
//

#ifndef VISMUT_READER_H
#define VISMUT_READER_H
#include "../../core/Vismut.h"


errno_t Reader_ReadFile(const char *filename, StringView *text);

#endif //VISMUT_READER_H
