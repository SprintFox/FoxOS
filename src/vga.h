#ifndef VGA_H
#define VGA_H

#include "stdint.h"

// VGA цвета
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

// Константы VGA
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

// Функции
void vga_init(void);
void vga_clear(void);
void vga_putchar(char c);
void vga_puts(const char* str);
void vga_write(const char* str);
void vga_set_color(uint8_t foreground, uint8_t background);
void vga_set_cursor(int x, int y);
void vga_get_cursor(int* x, int* y);
void vga_put_entry(int x, int y, char c);

// Форматированный вывод
void vga_printf(const char* format, ...);
void vga_put_dec(int64_t num);
void vga_put_hex(uint64_t num);
void vga_put_bin(uint64_t num);

#endif 