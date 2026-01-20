//
// Created by kir on 18.12.2025.
//

#ifndef VISMUT_ANSI_COLORS_H
#define VISMUT_ANSI_COLORS_H

#include <wchar.h>

#define ANSI_COLOR_RESET   L"\033[0m"
#define ANSI_COLOR_BLACK   L"\033[30m"
#define ANSI_COLOR_RED     L"\033[31m"
#define ANSI_COLOR_GREEN   L"\033[32m"
#define ANSI_COLOR_YELLOW  L"\033[33m"
#define ANSI_COLOR_BLUE    L"\033[34m"
#define ANSI_COLOR_MAGENTA L"\033[35m"
#define ANSI_COLOR_CYAN    L"\033[36m"
#define ANSI_COLOR_WHITE   L"\033[37m"
#define ANSI_COLOR_BRIGHT_BLACK   L"\033[90m"
#define ANSI_COLOR_BRIGHT_RED     L"\033[91m"
#define ANSI_COLOR_BRIGHT_GREEN   L"\033[92m"
#define ANSI_COLOR_BRIGHT_YELLOW  L"\033[93m"
#define ANSI_COLOR_BRIGHT_BLUE    L"\033[94m"
#define ANSI_COLOR_BRIGHT_MAGENTA L"\033[95m"
#define ANSI_COLOR_BRIGHT_CYAN    L"\033[96m"
#define ANSI_COLOR_BRIGHT_WHITE   L"\033[97m"

#define ANSI_BG_BLACK   L"\033[40m"
#define ANSI_BG_RED     L"\033[41m"
#define ANSI_BG_GREEN   L"\033[42m"
#define ANSI_BG_YELLOW  L"\033[43m"
#define ANSI_BG_BLUE    L"\033[44m"
#define ANSI_BG_MAGENTA L"\033[45m"
#define ANSI_BG_CYAN    L"\033[46m"
#define ANSI_BG_WHITE   L"\033[47m"
#define ANSI_BG_BRIGHT_BLACK   L"\033[100m"
#define ANSI_BG_BRIGHT_RED     L"\033[101m"
#define ANSI_BG_BRIGHT_GREEN   L"\033[102m"
#define ANSI_BG_BRIGHT_YELLOW  L"\033[103m"
#define ANSI_BG_BRIGHT_BLUE    L"\033[104m"
#define ANSI_BG_BRIGHT_MAGENTA L"\033[105m"
#define ANSI_BG_BRIGHT_CYAN    L"\033[106m"
#define ANSI_BG_BRIGHT_WHITE   L"\033[107m"

#define ANSI_BOLD       L"\033[1m"
#define ANSI_DIM        L"\033[2m"
#define ANSI_ITALIC     L"\033[3m"
#define ANSI_UNDERLINE  L"\033[4m"
#define ANSI_BLINK      L"\033[5m"
#define ANSI_REVERSE    L"\033[7m"
#define ANSI_HIDDEN     L"\033[8m"

void ansi_set_color(const wchar_t *color);

void ansi_set_bg_color(const wchar_t *color);

void ansi_set_style(const wchar_t *style);

void ansi_reset(void);

void ansi_print_color(const wchar_t *color, const wchar_t *text);

void ansi_print_bg(const wchar_t *bg_color, const wchar_t *text);

void ansi_print_styled(const wchar_t *color, const wchar_t *bg_color,
                       const wchar_t *style, const wchar_t *text);

#define PRINT_FG(COLOR, TEXT) ansi_print_color(COLOR, TEXT)
#define PRINT_BG(COLOR, TEXT) ansi_print_bg(COLOR, TEXT)
#define PRINT_STYLED(COLOR, BG_COLOR, STYLE, TEXT) ansi_print_styled(COLOR, BG_COLOR, STYLE, TEXT)
#define PRINT_COLORED(COLOR, BG_COLOR, TEXT) ansi_print_styled(COLOR, BG_COLOR, NULL, TEXT)

int ansi_supports_color(void);

void ansi_enable_color(int enable);

int win_ansi_init(void);

#endif //VISMUT_ANSI_COLORS_H
