//
// Created by kir on 18.12.2025.
//

#ifndef VISMUT_ANSI_COLORS_H
#define VISMUT_ANSI_COLORS_H

#include <wchar.h>

#define ANSI_COLOR_RESET   "\033[0m"
#define ANSI_COLOR_BLACK   "\033[30m"
#define ANSI_COLOR_RED     "\033[31m"
#define ANSI_COLOR_GREEN   "\033[32m"
#define ANSI_COLOR_YELLOW  "\033[33m"
#define ANSI_COLOR_BLUE    "\033[34m"
#define ANSI_COLOR_MAGENTA "\033[35m"
#define ANSI_COLOR_CYAN    "\033[36m"
#define ANSI_COLOR_WHITE   "\033[37m"
#define ANSI_COLOR_BRIGHT_BLACK   "\033[90m"
#define ANSI_COLOR_BRIGHT_RED     "\033[91m"
#define ANSI_COLOR_BRIGHT_GREEN   "\033[92m"
#define ANSI_COLOR_BRIGHT_YELLOW  "\033[93m"
#define ANSI_COLOR_BRIGHT_BLUE    "\033[94m"
#define ANSI_COLOR_BRIGHT_MAGENTA "\033[95m"
#define ANSI_COLOR_BRIGHT_CYAN    "\033[96m"
#define ANSI_COLOR_BRIGHT_WHITE   "\033[97m"

#define ANSI_BG_BLACK   "\033[40m"
#define ANSI_BG_RED     "\033[41m"
#define ANSI_BG_GREEN   "\033[42m"
#define ANSI_BG_YELLOW  "\033[43m"
#define ANSI_BG_BLUE    "\033[44m"
#define ANSI_BG_MAGENTA "\033[45m"
#define ANSI_BG_CYAN    "\033[46m"
#define ANSI_BG_WHITE   "\033[47m"
#define ANSI_BG_BRIGHT_BLACK   "\033[100m"
#define ANSI_BG_BRIGHT_RED     "\033[101m"
#define ANSI_BG_BRIGHT_GREEN   "\033[102m"
#define ANSI_BG_BRIGHT_YELLOW  "\033[103m"
#define ANSI_BG_BRIGHT_BLUE    "\033[104m"
#define ANSI_BG_BRIGHT_MAGENTA "\033[105m"
#define ANSI_BG_BRIGHT_CYAN    "\033[106m"
#define ANSI_BG_BRIGHT_WHITE   "\033[107m"

#define ANSI_BOLD       "\033[1m"
#define ANSI_DIM        "\033[2m"
#define ANSI_ITALIC     "\033[3m"
#define ANSI_UNDERLINE  "\033[4m"
#define ANSI_BLINK      "\033[5m"
#define ANSI_REVERSE    "\033[7m"
#define ANSI_HIDDEN     "\033[8m"

void ansi_set_color(const char *color);

void ansi_set_bg_color(const char *color);

void ansi_set_style(const char *style);

void ansi_reset(void);

void ansi_print_color(const char *color, const char *text);

void ansi_print_bg(const char *bg_color, const char *text);

void ansi_print_styled(const char *color, const char *bg_color,
                       const char *style, const char *text);

#define PRINT_FG(COLOR, TEXT) ansi_print_color(COLOR, TEXT)
#define PRINT_BG(COLOR, TEXT) ansi_print_bg(COLOR, TEXT)
#define PRINT_STYLED(COLOR, BG_COLOR, STYLE, TEXT) ansi_print_styled(COLOR, BG_COLOR, STYLE, TEXT)
#define PRINT_COLORED(COLOR, BG_COLOR, TEXT) ansi_print_styled(COLOR, BG_COLOR, NULL, TEXT)

int ansi_supports_color(void);

void ansi_enable_color(int enable);

int win_ansi_init(void);

#endif //VISMUT_ANSI_COLORS_H
