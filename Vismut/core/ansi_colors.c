#include "ansi_colors.h"

// ansi_colors.c
#include <wchar.h>
#include <stdio.h>
#include <Windows.h>

static int color_enabled = 1;
static int is_initialized = 0;
static int ansi_supported = 0;
static HANDLE hConsole = NULL;
static DWORD originalMode = 0;

int ansi_supports_color(void) {
#ifdef _WIN32
    return color_enabled;
#else
    return color_enabled && isatty(fileno(stdout));
#endif
}

void ansi_enable_color(const int enable) {
    color_enabled = enable;
}

void ansi_set_color(const char *color) {
    if (ansi_supports_color()) {
        fprintf(stdout, "%s", color);
    }
}

void ansi_set_bg_color(const char *color) {
    if (ansi_supports_color()) {
        fprintf(stdout, "%s", color);
    }
}

void ansi_set_style(const char *style) {
    if (ansi_supports_color()) {
        fprintf(stdout, "%s", style);
    }
}

void ansi_reset(void) {
    if (ansi_supports_color()) {
        fprintf(stdout, ANSI_COLOR_RESET);
    }
}

void ansi_print_color(const char *color, const char *text) {
    if (ansi_supports_color()) {
        fprintf(stdout, "%s%s" ANSI_COLOR_RESET, color, text);
    } else {
        fprintf(stdout, "%s", text);
    }
}

void ansi_print_bg(const char *bg_color, const char *text) {
    if (ansi_supports_color()) {
        fprintf(stdout, "%s%s" ANSI_COLOR_RESET, bg_color, text);
    } else {
        fprintf(stdout, "%s", text);
    }
}

void ansi_print_styled(const char *color, const char *bg_color,
                       const char *style, const char *text) {
    if (ansi_supports_color()) {
        if (style) fprintf(stdout, "%s", style);
        if (bg_color) fprintf(stdout, "%s", bg_color);
        if (color) fprintf(stdout, "%s", color);
        fprintf(stdout, "%s" ANSI_COLOR_RESET, text);
    } else {
        fprintf(stdout, "%s", text);
    }
}

int win_ansi_init(void) {
    if (is_initialized) {
        return ansi_supported;
    }

#ifdef _WIN32
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
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
        ansi_supported = 1;
    } else {
        // Если не удалось, пробуем альтернативный метод
        // Для Windows 10 до Creators Update (версия 1703)
        newMode = originalMode | 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
        ansi_supported = SetConsoleMode(hConsole, newMode);
    }
#else
    // На Unix-подобных системах ANSI коды работают по умолчанию
    ansi_supported = true;
#endif

    is_initialized = 1;
    return ansi_supported;
}
