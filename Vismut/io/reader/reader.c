#include "reader.h"
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <windows.h>

#include "../../core/errors/errors.h"


errno_t Reader_ReadFile(const char *filename, StringView *text) {
    FILE *file = NULL;
    errno_t err = 0;

    file = fopen((const char *) filename, "rb");
    if (file == NULL) {
        return VISMUT_ERROR_IO;
    }

    if ((err = fseek(file, 0, SEEK_END)) != 0) {
        return err;
    }

    const long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (file_size < 0) {
        fclose(file);
        return VISMUT_ERROR_IO;
    }

    uint8_t *raw_buffer = malloc(file_size + 1);
    if (raw_buffer == NULL) {
        fclose(file);
        return VISMUT_ERROR_ALLOC;
    }

    const size_t bytes_read = fread(raw_buffer, 1, file_size, file);
    fclose(file);

    if (bytes_read != (size_t) file_size) {
        free(raw_buffer);
        return VISMUT_ERROR_IO;
    }

    raw_buffer[bytes_read] = '\0';
    text->data = raw_buffer;
    text->length = bytes_read;

    return 0;
}
