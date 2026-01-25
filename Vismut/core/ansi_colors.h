//
// Created by kir on 18.12.2025.
//

#ifndef VISMUT_ANSI_COLORS_H
#define VISMUT_ANSI_COLORS_H
#include <stdint.h>
#include <stdio.h>

#define ANSI_RESET   "\033[0m\033[49m"
#define ANSI_BLACK_FG   "\033[30m"
#define ANSI_RED_FG     "\033[31m"
#define ANSI_GREEN_FG   "\033[32m"
#define ANSI_YELLOW_FG  "\033[33m"
#define ANSI_BLUE_FG    "\033[34m"
#define ANSI_MAGENTA_FG "\033[35m"
#define ANSI_CYAN_FG    "\033[36m"
#define ANSI_WHITE_FG   "\033[37m"
#define ANSI_BRIGHT_BLACK_FG   "\033[90m"
#define ANSI_BRIGHT_RED_FG     "\033[91m"
#define ANSI_BRIGHT_GREEN_FG   "\033[92m"
#define ANSI_BRIGHT_YELLOW_FG  "\033[93m"
#define ANSI_BRIGHT_BLUE_FG    "\033[94m"
#define ANSI_BRIGHT_MAGENTA_FG "\033[95m"
#define ANSI_BRIGHT_CYAN_FG    "\033[96m"
#define ANSI_BRIGHT_WHITE_FG   "\033[97m"

#define ANSI_BLACK_BG   "\033[40m"
#define ANSI_RED_BG     "\033[41m"
#define ANSI_GREEN_BG   "\033[42m"
#define ANSI_YELLOW_BG  "\033[43m"
#define ANSI_BLUE_BG    "\033[44m"
#define ANSI_MAGENTA_BG "\033[45m"
#define ANSI_CYAN_BG    "\033[46m"
#define ANSI_WHITE_BG   "\033[47m"
#define ANSI_BRIGHT_BLACK_BG   "\033[100m"
#define ANSI_BRIGHT_RED_BG     "\033[101m"
#define ANSI_BRIGHT_GREEN_BG   "\033[102m"
#define ANSI_BRIGHT_YELLOW_BG  "\033[103m"
#define ANSI_BRIGHT_BLUE_BG    "\033[104m"
#define ANSI_BRIGHT_MAGENTA_BG "\033[105m"
#define ANSI_BRIGHT_CYAN_BG    "\033[106m"
#define ANSI_BRIGHT_WHITE_BG   "\033[107m"

#define ANSI_BOLD       "\033[1m"
#define ANSI_DIM        "\033[2m"
#define ANSI_ITALIC     "\033[3m"
#define ANSI_UNDERLINE  "\033[4m"
#define ANSI_BLINK      "\033[5m"
#define ANSI_REVERSE    "\033[7m"
#define ANSI_HIDDEN     "\033[8m"

void ansi_set_color(FILE *, const char *color);

void ansi_reset(FILE *);

void ansi_print_with_style(FILE *file, const char *color, const char *text);

void ansi_print_with_style_formatted(FILE *file, const char *color, const char *format, ...);

#if !defined(ANSI_DEFAULT_OUT_FILE)
#define ANSI_DEFAULT_OUT_FILE stdout
#endif

#define SET_COLOR(COLOR) ansi_set_color(ANSI_DEFAULT_OUT_FILE, COLOR)
#define SET_COLOR2(COLOR1, COLOR2) ansi_set_color(ANSI_DEFAULT_OUT_FILE, COLOR1 COLOR2)
#define SET_COLOR3(COLOR1, COLOR2, COLOR3) ansi_set_color(ANSI_DEFAULT_OUT_FILE, COLOR1 COLOR2 COLOR3)

#define SET_FILE_COLOR(FILE, COLOR) ansi_set_color(FILE, COLOR)
#define SET_FILE_COLOR2(FILE, COLOR1, COLOR2) ansi_set_color(FILE, COLOR1 COLOR2)
#define SET_FILE_COLOR3(FILE, COLOR1, COLOR2, COLOR3) ansi_set_color(FILE, COLOR1 COLOR2 COLOR3)

#define RESET_COLOR() ansi_reset(ANSI_DEFAULT_OUT_FILE)
#define RESET_FILE_COLOR(FILE) ansi_reset(FILE)

#define PRINT_COLOR(COLOR, TEXT) ansi_print_with_style(ANSI_DEFAULT_OUT_FILE, COLOR, TEXT)
#define PRINT_2COLOR(COLOR1, COLOR2, TEXT) ansi_print_with_style(ANSI_DEFAULT_OUT_FILE, COLOR1 COLOR2, TEXT)
#define PRINT_3COLOR(COLOR1, COLOR2, COLOR3, TEXT) ansi_print_with_style(ANSI_DEFAULT_OUT_FILE, COLOR1 COLOR2 COLOR3, TEXT)

#define PRINTF_COLOR(COLOR, TEXT, ...) ansi_print_with_style_formatted(ANSI_DEFAULT_OUT_FILE, COLOR, TEXT, __VA_ARGS__)
#define PRINTF_2COLOR(COLOR1, COLOR2, TEXT, ...) ansi_print_with_style_formatted(ANSI_DEFAULT_OUT_FILE, COLOR1 COLOR2, TEXT, __VA_ARGS__)
#define PRINTF_3COLOR(COLOR1, COLOR2, COLOR3, TEXT, ...) ansi_print_with_style_formatted(ANSI_DEFAULT_OUT_FILE, COLOR1 COLOR2 COLOR3, TEXT, __VA_ARGS__)

#define FPRINT_COLOR(FILE, COLOR, TEXT) ansi_print_with_style(FILE, COLOR, TEXT)
#define FPRINT_2COLOR(FILE, COLOR1, COLOR2, TEXT) ansi_print_with_style(FILE, COLOR1 COLOR2, TEXT)
#define FPRINT_3COLOR(FILE, COLOR1, COLOR2, COLOR3, TEXT) ansi_print_with_style(FILE, COLOR1 COLOR2 COLOR3, TEXT)

#define FPRINTF_COLOR(FILE, COLOR, TEXT, ...) ansi_print_with_style_formatted(FILE, COLOR, TEXT, __VA_ARGS__)
#define FPRINTF_2COLOR(FILE, COLOR1, COLOR2, TEXT, ...) ansi_print_with_style_formatted(FILE, COLOR1 COLOR2, TEXT, __VA_ARGS__)
#define FPRINTF_3COLOR(FILE, COLOR1, COLOR2, COLOR3, TEXT, ...) ansi_print_with_style_formatted(FILE, COLOR1 COLOR2 COLOR3, TEXT, __VA_ARGS__)

int ansi_supports_color(FILE *);

void ansi_enable_color(int enable);

int win_ansi_init(FILE *);

#endif //VISMUT_ANSI_COLORS_H
