#include "terminal.h"
#include "keyboard.h"
#include "vga.h"
#include "fs.h"

// Объявления строковых функций
void strcpy(char* dest, const char* src);
void strcat(char* dest, const char* src);
void strncpy(char* dest, const char* src, uint32_t n);
int strncmp(const char* s1, const char* s2, uint32_t n);
int strcmp(const char* s1, const char* s2);

static char input_buffer[TERMINAL_BUFFER_SIZE];
static int buffer_pos = 0;
static int prompt_length;  // Длина промпта для защиты от стирания
static int cursor_pos = 0; // Позиция курсора в буфере

// Текущая директория (путь)
static char current_dir[MAX_FILENAME * 2] = "/";

#define CHAR_UP    1  // Ctrl-A
#define CHAR_DOWN  2  // Ctrl-B
#define CHAR_LEFT  3  // Ctrl-C
#define CHAR_RIGHT 4  // Ctrl-D
#define CHAR_DEL   5  // Ctrl-E

// Очистка буфера ввода
static void clear_buffer(void) {
    for (int i = 0; i < TERMINAL_BUFFER_SIZE; i++) {
        input_buffer[i] = 0;
    }
    buffer_pos = 0;
    cursor_pos = 0;
}

// Обновление позиции курсора на экране
static void update_cursor_position(void) {
    int x, y;
    vga_get_cursor(&x, &y);
    
    // Вычисляем новую позицию
    int new_x = (prompt_length + cursor_pos) % VGA_WIDTH;
    int new_y = y - ((prompt_length + buffer_pos) / VGA_WIDTH) + ((prompt_length + cursor_pos) / VGA_WIDTH);
    
    vga_set_cursor(new_x, new_y);
}

// Перерисовка строки ввода
static void redraw_input_line(void) {
    int x, y;
    vga_get_cursor(&x, &y);
    
    // Возвращаемся к началу строки ввода
    y -= (prompt_length + buffer_pos) / VGA_WIDTH;
    x = prompt_length % VGA_WIDTH;
    vga_set_cursor(x, y);
    
    // Очищаем текущую строку и один дополнительный символ
    for (int i = 0; i <= buffer_pos; i++) {
        vga_putchar(' ');
    }
    
    // Возвращаемся к началу и выводим содержимое буфера
    vga_set_cursor(x, y);
    for (int i = 0; i < buffer_pos; i++) {
        vga_putchar(input_buffer[i]);
    }
    
    // Восстанавливаем позицию курсора
    update_cursor_position();
}

// Разбор аргументов команды
static void parse_args(const char* cmd, char* arg1, char* arg2) {
    // Пропускаем имя команды
    while (*cmd && *cmd != ' ') cmd++;
    while (*cmd == ' ') cmd++;
    
    // Копируем первый аргумент
    while (*cmd && *cmd != ' ') {
        *arg1++ = *cmd++;
    }
    *arg1 = 0;
    
    // Пропускаем пробелы
    while (*cmd == ' ') cmd++;
    
    // Копируем второй аргумент
    while (*cmd) {
        *arg2++ = *cmd++;
    }
    *arg2 = 0;
}

// Построение полного пути
static void build_path(const char* relative_path, char* full_path) {
    if (relative_path[0] == '/') {
        strcpy(full_path, relative_path);
    } else {
        strcpy(full_path, current_dir);
        if (current_dir[1] != 0) {  // Если мы не в корневой директории
            strcat(full_path, "/");
        }
        strcat(full_path, relative_path);
    }
}

// Обработка команды
static void execute_command(void) {
    if (buffer_pos == 0) {
        return;
    }

    // Добавляем завершающий ноль
    input_buffer[buffer_pos] = '\0';

    // Буферы для аргументов
    char arg1[MAX_FILENAME] = {0};
    char arg2[MAX_FILENAME] = {0};
    char full_path[MAX_FILENAME * 2] = {0};

    // Сравниваем команды
    if (strcmp(input_buffer, "help") == 0) {
        vga_printf("Available commands:\n");
        vga_printf("  help     - Show this help\n");
        vga_printf("  clear    - Clear screen\n");
        vga_printf("  version  - Show version\n");
        vga_printf("  color    - Change text color\n");
        vga_printf("  ls       - List directory contents\n");
        vga_printf("  cd       - Change directory\n");
        vga_printf("  mkdir    - Create directory\n");
        vga_printf("  touch    - Create empty file\n");
        vga_printf("  rm       - Remove file or empty directory\n");
        vga_printf("  pwd      - Print working directory\n");
    }
    else if (strcmp(input_buffer, "clear") == 0) {
        vga_clear();
    }
    else if (strcmp(input_buffer, "exit") == 0) {
        vga_printf("Exiting FoxOS...\n");
        // Выключение компьютера через ACPI
        __asm__ volatile (
            // 1. Отключаем все прерывания
            "cli\n"
            // 2. Ждем завершения всех операций ввода-вывода
            "1:\n"
            "in $0x64, %al\n"
            "test $0x02, %al\n"
            "jnz 1b\n"
            // 3. Отправляем команду выключения в порт ACPI
            "mov $0x2000, %ax\n"
            "mov $0x604, %dx\n"
            "out %ax, %dx\n"
            // 4. Отправляем команду выключения питания
            "movw $0x5301, %ax\n"
            "movw $0x1004, %bx\n"
            "int $0x15\n"
            // 5. Устанавливаем режим выключения питания
            "movw $0x5307, %ax\n"
            "movw $0x1, %bx\n"
            "movw $0x3, %cx\n"
            "int $0x15\n"
            // 6. Если не удалось выключить, зацикливаемся
            "hlt\n"
            "jmp .\n"
        );
        return;
    }
    else if (strcmp(input_buffer, "test-formatting") == 0) {
        // Демонстрация различных форматов вывода
        vga_printf("Decimal numbers:\n");
        vga_printf("  Positive: %d\n", 12345);
        vga_printf("  Negative: %d\n", -9876);
        vga_printf("  Zero: %d\n", 0);
        vga_printf("  Large Positive: %d\n", 2147483647);
        vga_printf("  Large Negative: %d\n", -2147483648);
        vga_printf("  Very Large: %lld\n", 9223372036854775807LL);
        vga_printf("  Very Large Negative: %lld\n\n", (-9223372036854775807LL - 1));

        vga_printf("Other formats:\n");
        vga_printf("  Hexadecimal: %x\n", 0xDEADBEEF);
        vga_printf("  Binary: %b\n", 0b1010110);
        vga_printf("  Character: %c\n", 'A');
        vga_printf("  String: %s\n", "Hello, World!");
        vga_printf("  Mixed: Dec=%d, Hex=%x\n\n", 123, 123);
    }
    else if (strcmp(input_buffer, "test-colors") == 0) {
        // Демонстрация цветов
        vga_printf("\nAvailable colors:\n");
        
        // Красный текст
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_printf("RED ");
        
        // Зеленый текст
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_printf("GREEN ");
        
        // Синий текст
        vga_set_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
        vga_printf("BLUE ");
        
        // Голубой текст
        vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        vga_printf("CYAN ");
        
        // Пурпурный текст
        vga_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
        vga_printf("MAGENTA ");
        
        // Коричневый текст
        vga_set_color(VGA_COLOR_BROWN, VGA_COLOR_BLACK);
        vga_printf("BROWN ");
        
        // Белый текст
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_printf("WHITE\n");
    }
    else if (strcmp(input_buffer, "version") == 0) {
        vga_printf("FoxOS v0.1\n");
    }
    else if (strcmp(input_buffer, "color") == 0) {
        static uint8_t current_color = VGA_COLOR_LIGHT_GREEN;
        current_color = (current_color + 1) % 16;
        vga_set_color(current_color, VGA_COLOR_BLACK);
    }
    else if (strcmp(input_buffer, "pwd") == 0) {
        vga_printf("%s\n", current_dir);
    }
    else if (strncmp(input_buffer, "ls", 2) == 0) {
        parse_args(input_buffer, arg1, arg2);
        if (!arg1[0] || strcmp(arg1, ".") == 0) {
            // Если аргумент пустой или ".", используем текущую директорию
            strcpy(full_path, current_dir);
        } else {
            build_path(arg1, full_path);
        }
        
        char list_buffer[MAX_FILES * (MAX_FILENAME + 2)];
        int count = fs_list_dir(full_path, list_buffer, sizeof(list_buffer));
        
        if (count < 0) {
            vga_printf("Error: Cannot list directory\n");
        } else {
            vga_printf("%s", list_buffer);
        }
    }
    else if (strncmp(input_buffer, "cd", 2) == 0) {
        parse_args(input_buffer, arg1, arg2);
        if (!arg1[0]) {
            strcpy(current_dir, "/");
        } else {
            build_path(arg1, full_path);
            file_t* dir = fs_get_file(full_path);
            if (dir && dir->type == FILE_TYPE_DIR) {
                strcpy(current_dir, full_path);
            } else {
                vga_printf("Error: Directory not found\n");
            }
        }
    }
    else if (strncmp(input_buffer, "mkdir", 5) == 0) {
        parse_args(input_buffer, arg1, arg2);
        if (!arg1[0]) {
            vga_printf("Error: Directory name required\n");
        } else {
            build_path(arg1, full_path);
            if (fs_mkdir(full_path) < 0) {
                vga_printf("Error: Cannot create directory\n");
            }
        }
    }
    else if (strncmp(input_buffer, "touch", 5) == 0) {
        parse_args(input_buffer, arg1, arg2);
        if (!arg1[0]) {
            vga_printf("Error: File name required\n");
        } else {
            build_path(arg1, full_path);
            char parent_path[MAX_FILENAME * 2] = {0};
            char* last_slash = full_path;
            
            // Находим последний слеш
            for (char* p = full_path; *p; p++) {
                if (*p == '/') last_slash = p;
            }
            
            // Копируем родительский путь
            strncpy(parent_path, full_path, last_slash - full_path);
            if (!parent_path[0]) strcpy(parent_path, "/");
            
            // Получаем индекс родительской директории
            int parent_index = fs_parse_path(parent_path);
            if (parent_index >= 0) {
                if (fs_create_file(last_slash + 1, FILE_TYPE_FILE, parent_index) < 0) {
                    vga_printf("Error: Cannot create file\n");
                }
            } else {
                vga_printf("Error: Invalid path\n");
            }
        }
    }
    else if (strncmp(input_buffer, "rm", 2) == 0) {
        parse_args(input_buffer, arg1, arg2);
        if (!arg1[0]) {
            vga_printf("Error: File/directory name required\n");
        } else {
            build_path(arg1, full_path);
            if (fs_delete_file(full_path) < 0) {
                vga_printf("Error: Cannot remove file/directory\n");
            }
        }
    }
    else {
        vga_printf("Unknown command: %s\n", input_buffer);
    }

}

void terminal_init(void) {
    keyboard_init();
    fs_init();  // Инициализируем файловую систему
    clear_buffer();
    prompt_length = sizeof(TERMINAL_PROMPT) - 1;  // -1 чтобы не учитывать завершающий ноль
    vga_printf(TERMINAL_PROMPT);
}

void terminal_run(void) {
    char c = keyboard_read();

    // Пропускаем нулевые символы (отпускание клавиш и модификаторы)
    if (c == 0) {
        return;
    }

    // Обработка специальных символов
    switch (c) {
        case CHAR_LEFT:
            if (cursor_pos > 0) {
                cursor_pos--;
                update_cursor_position();
            }
            return;

        case CHAR_RIGHT:
            if (cursor_pos < buffer_pos) {
                cursor_pos++;
                update_cursor_position();
            }
            return;

        case CHAR_DEL:
            if (cursor_pos < buffer_pos) {
                // Сдвигаем символы влево начиная с позиции после курсора
                for (int i = cursor_pos; i < buffer_pos - 1; i++) {
                    input_buffer[i] = input_buffer[i + 1];
                }
                buffer_pos--;
                input_buffer[buffer_pos] = 0;
                redraw_input_line();
            }
            return;

        case '\n':
            vga_putchar('\n');
            execute_command();
            clear_buffer();
            vga_printf(TERMINAL_PROMPT);
            return;

        case '\b':
            if (cursor_pos > 0) {
                // Сдвигаем символы влево
                for (int i = cursor_pos - 1; i < buffer_pos - 1; i++) {
                    input_buffer[i] = input_buffer[i + 1];
                }
                buffer_pos--;
                cursor_pos--;
                input_buffer[buffer_pos] = 0;
                redraw_input_line();
            }
            return;
    }

    // Добавление обычного символа
    if (buffer_pos < TERMINAL_BUFFER_SIZE - 1 && c >= ' ' && c <= '~') {
        // Сдвигаем символы вправо
        for (int i = buffer_pos; i > cursor_pos; i--) {
            input_buffer[i] = input_buffer[i - 1];
        }
        input_buffer[cursor_pos] = c;
        buffer_pos++;
        cursor_pos++;
        redraw_input_line();
    }
} 