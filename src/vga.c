#include "vga.h"
#include "io.h"

static uint16_t* const vga_buffer = (uint16_t*)VGA_MEMORY;
static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t vga_color = 0;

// Создает цветовой атрибут из цветов переднего и заднего плана
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

// Создает символ VGA (2 байта - символ и атрибут)
static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | (uint16_t)color << 8;
}

// Прокрутка экрана вверх на одну строку
static void vga_scroll(void) {
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    // Очистка последней строки
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT-1) * VGA_WIDTH + x] = vga_entry(' ', vga_color);
    }
}

void vga_init(void) {
    vga_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
}

void vga_clear(void) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_entry(' ', vga_color);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
    vga_set_cursor(0, 0);
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            // Очищаем символ в текущей позиции
            vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(' ', vga_color);
            // Сразу обновляем позицию курсора
            vga_set_cursor(cursor_x, cursor_y);
            return;  // Выходим, чтобы избежать повторного обновления курсора
        } else if (cursor_y > 0) {
            // Если мы в начале строки и есть предыдущая строка
            cursor_y--;
            cursor_x = VGA_WIDTH - 1;
            vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(' ', vga_color);
            vga_set_cursor(cursor_x, cursor_y);
            return;
        }
    } else {
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(c, vga_color);
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= VGA_HEIGHT) {
        vga_scroll();
        cursor_y = VGA_HEIGHT - 1;
    }

    vga_set_cursor(cursor_x, cursor_y);
}

void vga_write(const char* str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

void vga_set_color(uint8_t foreground, uint8_t background) {
    vga_color = vga_entry_color(foreground, background);
}

void vga_set_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;
    
    // Управляющие порты курсора VGA
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
    
    cursor_x = x;
    cursor_y = y;
}

// Вспомогательная функция для вывода числа в десятичном формате
void vga_put_dec(int64_t num) {
    // Специальная обработка для минимального значения int64_t
    if (num == INT64_MIN) {
        vga_write("-9223372036854775808");
        return;
    }

    char buf[32];
    int i = 0;
    int is_negative = 0;

    // Проверка на отрицательное число
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    // Обработка случая с нулем
    if (num == 0) {
        buf[i++] = '0';
    } else {
        // Преобразуем число в строку (в обратном порядке)
        while (num > 0) {
            buf[i++] = '0' + (num % 10);
            num /= 10;
        }
    }

    // Добавляем знак минус для отрицательных чисел
    if (is_negative) {
        vga_putchar('-');
    }

    // Выводим строку в правильном порядке
    while (i > 0) {
        vga_putchar(buf[--i]);
    }
}

// Вспомогательная функция для вывода числа в шестнадцатеричном формате
void vga_put_hex(uint64_t num) {
    char buf[32];
    int i = 0;
    const char hex_digits[] = "0123456789ABCDEF";

    // Преобразуем число в строку (в обратном порядке)
    do {
        buf[i++] = hex_digits[num & 0xF];
        num = num >> 4;
    } while (num > 0);

    // Выводим 0x префикс
    vga_write("0x");

    // Выводим строку в правильном порядке
    while (i > 0) {
        vga_putchar(buf[--i]);
    }
}

// Вспомогательная функция для вывода числа в двоичном формате
void vga_put_bin(uint64_t num) {
    char buf[65];
    int i = 0;

    // Преобразуем число в строку (в обратном порядке)
    do {
        buf[i++] = '0' + (num & 1);
        num = num >> 1;
    } while (num > 0);

    // Выводим 0b префикс
    vga_write("0b");

    // Выводим строку в правильном порядке
    while (i > 0) {
        vga_putchar(buf[--i]);
    }
}

// Форматированный вывод
void vga_printf(const char* format, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, format);

    while (*format) {
        if (*format == '%') {
            format++;
            // Проверяем модификатор ll
            if (*format == 'l' && *(format + 1) == 'l') {
                format += 2;
                if (*format == 'd') {
                    vga_put_dec(__builtin_va_arg(args, int64_t));
                }
            } else {
                switch (*format) {
                    case 'd': // Десятичное число
                        vga_put_dec(__builtin_va_arg(args, int));
                        break;
                    case 'x': // Шестнадцатеричное число
                        vga_put_hex(__builtin_va_arg(args, uint64_t));
                        break;
                    case 'b': // Двоичное число
                        vga_put_bin(__builtin_va_arg(args, uint64_t));
                        break;
                    case 's': // Строка
                        vga_write(__builtin_va_arg(args, const char*));
                        break;
                    case 'c': // Символ
                        vga_putchar(__builtin_va_arg(args, int));
                        break;
                    case '%': // Символ %
                        vga_putchar('%');
                        break;
                    default:
                        vga_putchar('%');
                        vga_putchar(*format);
                }
            }
        } else {
            vga_putchar(*format);
        }
        format++;
    }

    __builtin_va_end(args);
}

// Получение текущей позиции курсора
void vga_get_cursor(int* x, int* y) {
    *x = cursor_x;
    *y = cursor_y;
}

// Запись символа в конкретную позицию
void vga_put_entry(int x, int y, char c) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        vga_buffer[y * VGA_WIDTH + x] = vga_entry(c, vga_color);
    }
}

void vga_puts(const char* str) {
    while (*str) {
        vga_putchar(*str++);
    }
}