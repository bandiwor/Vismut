#include "find_position.h"

TextPosition FindPosition(const uint8_t *source, const uint8_t *source_end, const uint8_t *ptr) {
    if (!source || !source_end || !ptr || ptr < source || ptr >= source_end) {
        return (TextPosition){0};
    }

    size_t line = 1;
    const uint8_t *line_start = source;
    const uint8_t *current = source;

    while (current < ptr) {
        if (*current == '\n') {
            ++line;
            line_start = current + 1;
        }
        ++current;
    }

    const uint8_t *line_end = ptr;
    while (line_end < source_end && *line_end != '\n' && *line_end != '\r' && *line_end != '\0') {
        line_end++;
    }

    return (TextPosition){
        .line_start = line_start,
        .line_end = line_end,
        .line = line,
        .column = (ptr - line_start) + 1
    };
}
