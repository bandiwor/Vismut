#include "module_name.h"

static int is_valid_identifier_char(const uint8_t c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

static int should_replace_char(const uint8_t c) {
    return c == '.' || c == '-' || c == ' ' || c == '+' || c == '~' ||
           c == '!' || c == '@' || c == '#' || c == '$' || c == '%' ||
           c == '^' || c == '&' || c == '*' || c == '(' || c == ')' ||
           c == '=' || c == '[' || c == ']' || c == '{' || c == '}' ||
           c == '|' || c == ';' || c == ':' || c == '\'' || c == '"' ||
           c == '<' || c == '>' || c == ',' || c == '?' || c == '`';
}

uint8_t *CreateModuleName(Arena *arena, const uint8_t *filename, const int length) {
    if (!filename || length <= 0) return NULL;

    const uint8_t *end = filename + length;

    const uint8_t *last_sep = NULL;
    for (const uint8_t *p = filename; p < end; p++) {
        if (*p == '/' || *p == '\\') {
            last_sep = p;
        }
    }

    const uint8_t *name_start = last_sep ? last_sep + 1 : filename;

    const uint8_t *name_end = end;
    if (end - name_start >= 7) {
        if (__builtin_strncmp((const char *) end - 7, ".vismut", 7) == 0) {
            name_end = end - 7;
        }
    }

    uint8_t *result = Arena_Array(arena, uint8_t, (name_end - name_start) + 1);
    if (!result) return NULL;

    int idx = 0;
    int last_was_replacement = 0;

    for (const uint8_t *p = name_start; p < name_end; p++) {
        const uint8_t c = *p;

        if (is_valid_identifier_char(c)) {
            result[idx++] = c;
            last_was_replacement = 0;
        } else if (should_replace_char(c)) {
            if (!last_was_replacement && idx > 0) {
                result[idx++] = '_';
                last_was_replacement = 1;
            }
        }
    }

    while (idx > 0 && result[idx - 1] == '_') {
        idx--;
    }

    if (idx == 0) {
        return NULL;
    }

    result[idx] = '\0';

    return result;
}
