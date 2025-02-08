#ifndef FS_H
#define FS_H

#include "stdint.h"

// Максимальная длина имени файла
#define MAX_FILENAME 256
// Максимальный размер файла
#define MAX_FILE_SIZE 4096
// Максимальное количество файлов
#define MAX_FILES 256

// Тип файла
typedef enum {
    FILE_TYPE_NONE = 0,
    FILE_TYPE_FILE = 1,
    FILE_TYPE_DIR = 2
} file_type_t;

// Структура файла
typedef struct {
    char name[MAX_FILENAME];
    file_type_t type;
    uint32_t size;
    uint32_t parent_index;
    uint8_t data[MAX_FILE_SIZE];
} file_t;

// Структура суперблока
typedef struct {
    uint32_t magic;          // Магическое число для проверки
    uint32_t version;        // Версия файловой системы
    uint32_t file_count;     // Количество файлов
    uint32_t first_data_sector; // Первый сектор данных
} __attribute__((packed)) superblock_t;

#define FS_MAGIC 0x534F584F  // "FOXS" в hex
#define FS_VERSION 1
#define FS_SUPERBLOCK_SECTOR 2048  // После загрузчика и ядра
#define FS_DATA_START_SECTOR 2049   // Сразу после суперблока

// Функции файловой системы
void fs_init(void);
int fs_create_file(const char* name, file_type_t type, uint32_t parent_index);
file_t* fs_get_file(const char* path);
int fs_write(const char* path, const uint8_t* data, uint32_t size);
int fs_read(const char* path, uint8_t* buffer, uint32_t size);
int fs_delete_file(const char* path);
int fs_mkdir(const char* path);
int fs_list_dir(const char* path, char* buffer, uint32_t buffer_size);

// Новые функции для работы с диском
int fs_save(void);
int fs_load(void);

// Вспомогательные функции
int fs_parse_path(const char* path);

#endif 