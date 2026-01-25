#include "ansi_colors.h"

// ansi_colors.c
#include <stdio.h>
#include <Windows.h>

static int color_enabled = 1;
static int is_initialized = 0;
static int ansi_supported = 0;
static HANDLE hConsole = NULL;
static DWORD originalMode = 0;

int ansi_supports_color(FILE *file) {
#ifdef _WIN32
    return color_enabled;
#else
    return color_enabled && isatty(fileno(file));
#endif
}

void ansi_enable_color(const int enable) {
    color_enabled = enable;
}

void ansi_set_color(FILE *file, const char *color) {
    if (ansi_supports_color(file)) {
        fprintf(file, "%s", color);
    }
}

void ansi_reset(FILE *file) {
    if (ansi_supports_color(file)) {
        fprintf(file, ANSI_RESET);
    }
}

void ansi_print_with_style(FILE *file, const char *color, const char *text) {
    if (ansi_supports_color(file)) {
        fprintf(file, "%s%s" ANSI_RESET, color, text);
    } else {
        fprintf(file, "%s", text);
    }
}

void ansi_print_with_style_formatted(FILE *file, const char *color, const char *format, ...) {
    va_list args;
    va_start(args, format);

    if (ansi_supports_color(file)) {
        fprintf(file, "%s", color);
        vfprintf(file, format, args);
        fprintf(file, ANSI_RESET);
    } else {
        vfprintf(file, format, args);
    }

    va_end(args);
}

int win_ansi_init(FILE *file) {
    if (is_initialized) {
        return ansi_supported;
    }

#ifdef _WIN32
    if (file != stdout && file != stderr) {
        is_initialized = 1;
        return ansi_supported = 0;
    }

    hConsole = GetStdHandle(file == stdout ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) {
        is_initialized = 1;
        ansi_supported = 0;
        return 0;
    }

    if (!GetConsoleMode(hConsole, &originalMode)) {
        is_initialized = 1;
        ansi_supported = 0;
        return 0;
    }

    DWORD newMode = originalMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (SetConsoleMode(hConsole, newMode)) {
        return 1;
    }

    newMode = ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    return SetConsoleMode(hConsole, newMode);
#else
    // На Unix-подобных системах ANSI коды работают по умолчанию
    ansi_supported = true;
#endif

    is_initialized = 1;
    return ansi_supported;
}
