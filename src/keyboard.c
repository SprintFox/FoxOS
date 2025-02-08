#include "keyboard.h"
#include "io.h"

// Карта символов (без shift)
static const char scancode_to_char[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

// Карта символов с shift
static const char scancode_to_char_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

// Специальные символы для стрелок
#define CHAR_UP    1  // Ctrl-A
#define CHAR_DOWN  2  // Ctrl-B
#define CHAR_LEFT  3  // Ctrl-C
#define CHAR_RIGHT 4  // Ctrl-D
#define CHAR_DEL   5  // Ctrl-E

static uint8_t shift_pressed = 0;

// Ожидание готовности клавиатуры
void keyboard_wait(void) {
    while (inb(0x64) & 2);
}

// Чтение скан-кода
uint8_t keyboard_get_scancode(void) {
    keyboard_wait();
    return inb(0x60);
}

// Инициализация клавиатуры
void keyboard_init(void) {
    // Очищаем буфер клавиатуры
    while (inb(0x64) & 1) {
        inb(0x60);
    }
}

// Чтение символа с клавиатуры
char keyboard_read(void) {
    uint8_t scancode;
    char c = 0;
    static uint8_t last_scancode = 0;
    static uint8_t key_held = 0;

    scancode = keyboard_get_scancode();

    // Обработка shift
    if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
        shift_pressed = 1;
        return 0;
    }
    if (scancode == (KEY_LSHIFT | 0x80) || scancode == (KEY_RSHIFT | 0x80)) {
        shift_pressed = 0;
        return 0;
    }

    // Если это отпускание клавиши
    if (scancode & 0x80) {
        // Сбрасываем флаги для этой клавиши
        if ((scancode & 0x7F) == last_scancode) {
            last_scancode = 0;
            key_held = 0;
        }
        return 0;
    }

    // Обработка стрелок и специальных клавиш
    switch (scancode) {
        case KEY_UP:
            c = CHAR_UP;
            break;
        case KEY_DOWN:
            c = CHAR_DOWN;
            break;
        case KEY_LEFT:
            c = CHAR_LEFT;
            break;
        case KEY_RIGHT:
            c = CHAR_RIGHT;
            break;
        case KEY_DELETE:
            c = CHAR_DEL;
            break;
        default:
            // Преобразуем скан-код в символ
            if (scancode < sizeof(scancode_to_char)) {
                c = shift_pressed ? scancode_to_char_shift[scancode] 
                                : scancode_to_char[scancode];
            }
            break;
    }

    // Если клавиша уже нажата, игнорируем повторы
    if (scancode == last_scancode && key_held) {
        return 0;
    }
    last_scancode = scancode;
    key_held = 1;

    return c;
} 