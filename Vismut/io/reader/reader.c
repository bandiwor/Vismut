#include "reader.h"
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <windows.h>

#include "../../core/errors/errors.h"


errno_t Reader_ReadFile(const wchar_t *filename, FileText *text) {
    FILE *file = NULL;
    errno_t err = 0;

    if ((err = _wfopen_s(&file, filename, L"r")) != 0) {
        return err;
    }
    if ((err = fseeko64(file, 0, SEEK_END)) != 0) {
        return err;
    }

    const int64_t file_size = (int64_t) ftello64(file);
    fseeko64(file, 0, SEEK_SET);
    if (file_size < 0) {
        fclose(file);
        return VISMUT_ERROR_IO;
    }

    char *raw_buffer = malloc(file_size + 1);
    if (raw_buffer == NULL) {
        fclose(file);
        return VISMUT_ERROR_ALLOC;
    }

    const size_t bytes_read = fread(raw_buffer, 1, file_size, file);
    fclose(file);

    if (bytes_read > (size_t) file_size) {
        free(raw_buffer);
        return VISMUT_ERROR_IO;
    }

    raw_buffer[bytes_read] = '\0';

    const int wide_chars_needed = MultiByteToWideChar(
        CP_UTF8, 0, raw_buffer, -1, NULL, 0);

    if (wide_chars_needed <= 0) {
        free(raw_buffer);
        return VISMUT_ERROR_ENCODING;
    }

    wchar_t *wide_buffer = malloc(wide_chars_needed * sizeof(wchar_t));
    if (wide_buffer == NULL) {
        free(raw_buffer);
        return VISMUT_ERROR_ALLOC;
    }

    const int converted = MultiByteToWideChar(
        CP_UTF8, 0, raw_buffer, -1, wide_buffer, wide_chars_needed);

    free(raw_buffer);

    if (converted <= 0) {
        free(wide_buffer);
        return VISMUT_ERROR_ENCODING;
    }

    text->data = wide_buffer;
    text->length = wcslen(wide_buffer);

    return 0;
}
