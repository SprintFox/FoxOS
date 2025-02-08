#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "stdint.h"

// Скан-коды клавиш
#define KEY_ENTER       0x1C
#define KEY_BACKSPACE   0x0E
#define KEY_LSHIFT      0x2A
#define KEY_RSHIFT      0x36

// Скан-коды стрелок
#define KEY_UP          0x48
#define KEY_DOWN        0x50
#define KEY_LEFT        0x4B
#define KEY_RIGHT       0x4D
#define KEY_DELETE      0x53

// Функции
void keyboard_init(void);
char keyboard_read(void);
uint8_t keyboard_get_scancode(void);
void keyboard_wait(void);

#endif 