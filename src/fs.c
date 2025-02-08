#include "fs.h"
#include "vga.h"  // Добавляем для вывода отладочной информации

// Массив всех файлов
static file_t files[MAX_FILES];
// Текущее количество файлов
static uint32_t file_count = 0;
// Номер порта SATA диска
static uint32_t disk_port = 0;

// Вспомогательная функция для копирования строк
void strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = 0;
}

// Вспомогательная функция для сравнения строк
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// Вспомогательная функция для получения длины строки
uint32_t strlen(const char* str) {
    const char* s = str;
    while (*s) s++;
    return s - str;
}

// Вспомогательная функция для конкатенации строк
void strcat(char* dest, const char* src) {
    dest += strlen(dest);
    strcpy(dest, src);
}

// Вспомогательная функция для копирования n символов
void strncpy(char* dest, const char* src, uint32_t n) {
    uint32_t i;
    for (i = 0; i < n && src[i]; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = 0;
    }
}

// Вспомогательная функция для сравнения начала строки
int strncmp(const char* s1, const char* s2, uint32_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    return n ? *(const unsigned char*)s1 - *(const unsigned char*)s2 : 0;
}

// Вспомогательная функция для поиска следующего компонента пути
static const char* get_next_path_component(const char* path, char* component) {
    // Пропускаем начальные слеши
    while (*path == '/') path++;
    
    // Копируем символы до следующего слеша или конца строки
    while (*path && *path != '/') {
        *component++ = *path++;
    }
    *component = 0;
    
    return *path ? path : NULL;
}

// Сохранение файловой системы на диск
int fs_save(void) {
    
    return 1;
}

// Загрузка файловой системы с диска
int fs_load(void) {
    
    return 1;
}

void fs_init(void) {
    // Если не удалось, создаем новую
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        files[i].type = FILE_TYPE_NONE;
        files[i].size = 0;
        files[i].name[0] = 0;
        files[i].parent_index = -1;
    }
    
    // Создаем корневую директорию
    files[0].name[0] = 0;  // Пустое имя для корневой директории
    files[0].type = FILE_TYPE_DIR;
    files[0].size = 0;
    files[0].parent_index = 0;  // Корень является родителем для самого себя
    file_count = 1;
}

int fs_create_file(const char* name, file_type_t type, uint32_t parent_index) {
    if (file_count >= MAX_FILES) {
        return -1;  // Нет свободного места
    }
    
    // Проверяем, что родительская директория существует и является директорией
    if (parent_index >= MAX_FILES || files[parent_index].type != FILE_TYPE_DIR) {
        return -1;
    }
    
    // Проверяем, что файл с таким именем не существует в этой директории
    for (uint32_t i = 0; i < file_count; i++) {
        if (files[i].parent_index == parent_index && strcmp(files[i].name, name) == 0) {
            return -1;  // Файл уже существует
        }
    }
    
    // Создаем новый файл
    uint32_t index = file_count++;
    strcpy(files[index].name, name);
    files[index].type = type;
    files[index].size = 0;
    files[index].parent_index = parent_index;
    
    // Сохраняем изменения на диск
    fs_save();
    
    return index;
}

int fs_parse_path(const char* path) {
    char component[MAX_FILENAME];
    uint32_t current_index = 0;  // Начинаем с корневой директории
    
    // Если путь пустой или это корень, возвращаем индекс корневой директории
    if (!*path || (*path == '/' && !*(path + 1))) {
        return 0;
    }
    
    // Разбираем путь по компонентам
    while (path) {
        path = get_next_path_component(path, component);
        if (!component[0]) continue;  // Пустой компонент
        
        // Ищем компонент в текущей директории
        int found = -1;
        for (uint32_t i = 0; i < file_count; i++) {
            if (files[i].parent_index == current_index && strcmp(files[i].name, component) == 0) {
                found = i;
                break;
            }
        }
        
        if (found == -1) {
            return -1;  // Компонент не найден
        }
        current_index = found;
    }
    
    return current_index;
}

file_t* fs_get_file(const char* path) {
    int index = fs_parse_path(path);
    if (index < 0) {
        return NULL;
    }
    return &files[index];
}

int fs_write(const char* path, const uint8_t* data, uint32_t size) {
    file_t* file = fs_get_file(path);
    if (!file || file->type != FILE_TYPE_FILE) {
        return -1;
    }
    
    // Проверяем размер
    if (size > MAX_FILE_SIZE) {
        size = MAX_FILE_SIZE;
    }
    
    // Копируем данные
    for (uint32_t i = 0; i < size; i++) {
        file->data[i] = data[i];
    }
    file->size = size;
    
    // Сохраняем изменения на диск
    fs_save();
    
    return size;
}

int fs_read(const char* path, uint8_t* buffer, uint32_t size) {
    file_t* file = fs_get_file(path);
    if (!file || file->type != FILE_TYPE_FILE) {
        return -1;
    }
    
    // Проверяем размер
    if (size > file->size) {
        size = file->size;
    }
    
    // Копируем данные
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = file->data[i];
    }
    
    return size;
}

int fs_delete_file(const char* path) {
    int index = fs_parse_path(path);
    if (index <= 0) {  // Не позволяем удалять корневую директорию
        return -1;
    }
    
    // Проверяем, что это не директория с файлами
    if (files[index].type == FILE_TYPE_DIR) {
        for (uint32_t i = 0; i < file_count; i++) {
            if (files[i].parent_index == index) {
                return -1;  // Директория не пуста
            }
        }
    }
    
    // Удаляем файл, сдвигая все последующие файлы
    for (uint32_t i = index; i < file_count - 1; i++) {
        files[i] = files[i + 1];
        // Обновляем parent_index для файлов, которые ссылались на сдвинутые файлы
        for (uint32_t j = 0; j < file_count; j++) {
            if (files[j].parent_index > i) {
                files[j].parent_index--;
            }
        }
    }
    
    file_count--;
    
    // Сохраняем изменения на диск
    fs_save();
    
    return 0;
}

int fs_mkdir(const char* path) {
    // Находим родительскую директорию
    char parent_path[MAX_FILENAME * 2] = {0};
    char dir_name[MAX_FILENAME] = {0};
    const char* last_slash = path;
    
    // Ищем последний слеш в пути
    for (const char* p = path; *p; p++) {
        if (*p == '/') last_slash = p;
    }
    
    // Копируем путь до последнего слеша
    for (const char* p = path; p < last_slash; p++) {
        parent_path[p - path] = *p;
    }
    if (!parent_path[0]) {
        strcpy(parent_path, "/");
    }
    
    // Копируем имя новой директории
    strcpy(dir_name, last_slash + 1);
    if (!dir_name[0]) {
        return -1;  // Пустое имя директории
    }
    
    // Получаем индекс родительской директории
    int parent_index = fs_parse_path(parent_path);
    if (parent_index < 0) {
        return -1;
    }
    
    // Создаем новую директорию
    int result = fs_create_file(dir_name, FILE_TYPE_DIR, parent_index);
    if (result >= 0) {
        fs_save(); // Сохраняем изменения на диск
    }
    return result;
}

int fs_list_dir(const char* path, char* buffer, uint32_t buffer_size) {
    int dir_index = fs_parse_path(path);
    if (dir_index < 0 || dir_index >= MAX_FILES || files[dir_index].type != FILE_TYPE_DIR) {
        return -1;
    }
    
    char* current_pos = buffer;
    int count = 0;
    
    // Перебираем все файлы, ищем те, которые находятся в данной директории
    for (uint32_t i = 0; i < file_count; i++) {
        // Пропускаем корневую директорию
        if (i == 0) continue;
        
        if (files[i].parent_index == dir_index) {
            uint32_t remaining = buffer_size - (current_pos - buffer);
            if (remaining < MAX_FILENAME + 3) {  // +3 для возможного добавления "/\n"
                break;
            }
            
            strcpy(current_pos, files[i].name);
            current_pos += strlen(files[i].name);
            
            if (files[i].type == FILE_TYPE_DIR) {
                *current_pos++ = '/';
            }
            *current_pos++ = '\n';
            count++;
        }
    }
    
    *current_pos = 0;  // Завершающий ноль
    return count;
}

int fs_create(const char* path) {
    // Находим родительскую директорию
    char parent_path[MAX_FILENAME * 2] = {0};
    char file_name[MAX_FILENAME] = {0};
    const char* last_slash = path;
    
    // Ищем последний слеш в пути
    for (const char* p = path; *p; p++) {
        if (*p == '/') last_slash = p;
    }
    
    // Копируем путь до последнего слеша
    for (const char* p = path; p < last_slash; p++) {
        parent_path[p - path] = *p;
    }
    if (!parent_path[0]) {
        strcpy(parent_path, "/");
    }
    
    // Копируем имя нового файла
    strcpy(file_name, last_slash + 1);
    if (!file_name[0]) {
        return -1;  // Пустое имя файла
    }
    
    // Получаем индекс родительской директории
    int parent_index = fs_parse_path(parent_path);
    if (parent_index < 0) {
        return -1;
    }
    
    // Создаем новый файл
    int result = fs_create_file(file_name, FILE_TYPE_FILE, parent_index);
    if (result >= 0) {
        fs_save(); // Сохраняем изменения на диск
    }
    return result;
}

int fs_delete(const char* path) {
    int idx = fs_parse_path(path);
    if (idx < 0) {
        return -1;
    }
    
    // Нельзя удалить корневую директорию
    if (idx == 0) {
        return -1;
    }
    
    // Очищаем файл/директорию
    files[idx].name[0] = 0;
    files[idx].type = FILE_TYPE_NONE;
    files[idx].size = 0;
    files[idx].parent_index = -1;
    
    fs_save(); // Сохраняем изменения на диск
    return 0;
} 