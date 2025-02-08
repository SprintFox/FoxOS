#include <stdint.h>

// Базовые типы UEFI
typedef uint64_t EFI_STATUS;
typedef void* EFI_HANDLE;
typedef uint32_t EFI_TPL;
typedef uint64_t EFI_PHYSICAL_ADDRESS;

// Макрос для расчета страниц памяти
#define EFI_SIZE_TO_PAGES(size) (((size) + 0xFFF) >> 12)

#define EFI_ERROR(status) ((status) != 0)

// Структура EFI_GUID
typedef struct {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[8];
} EFI_GUID;

// Протоколы
#define EFI_LOADED_IMAGE_PROTOCOL_GUID {0x5B1B31A1,0x9562,0x11d2,{0x8E,0x3F,0x00,0xA0,0xC9,0x69,0x72,0x3B}}
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID {0x0964e5b2,0x6459,0x11d2,{0x8E,0x39,0x00,0xA0,0xC9,0x69,0x72,0x3B}}
#define EFI_FILE_INFO_GUID {0x09576e92,0x6d3f,0x11d2,{0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}}

// Структура текстового вывода
typedef struct {
    void* Reset;
    EFI_STATUS (*OutputString)(void* This, uint16_t* String);
    void* TestString;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

// Структуры протоколов
typedef struct {
    uint64_t Revision;
    EFI_HANDLE ParentHandle;
} EFI_LOADED_IMAGE_PROTOCOL;

typedef struct {
    uint64_t Revision;
    EFI_STATUS (*OpenVolume)(void* This, void** Root);
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

typedef struct {
    uint64_t Revision;
    EFI_STATUS (*Open)(void* This, void** NewHandle, uint16_t* FileName, uint64_t OpenMode, uint64_t Attributes);
    EFI_STATUS (*Close)(void* This);
    EFI_STATUS (*Read)(void* This, uint64_t* BufferSize, void* Buffer);
    EFI_STATUS (*GetInfo)(void* This, EFI_GUID* InformationType, uint64_t* BufferSize, void* Buffer);
} EFI_FILE_PROTOCOL;

// Структура системной таблицы
typedef struct {
    char _pad[52];
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut;
    void* BootServices;
} EFI_SYSTEM_TABLE;

// Структура Boot Services
typedef struct {
    EFI_STATUS (*HandleProtocol)(EFI_HANDLE Handle, EFI_GUID* Protocol, void** Interface);
    EFI_STATUS (*AllocatePages)(uint32_t Type, uint32_t MemoryType, uint64_t Pages, EFI_PHYSICAL_ADDRESS* Memory);
    EFI_STATUS (*ExitBootServices)(EFI_HANDLE ImageHandle, uint64_t MapKey);
} EFI_BOOT_SERVICES;

// Исправить функцию Print
static void Print(EFI_SYSTEM_TABLE* st, const char* str) {
    uint16_t buf[256] = {0};
    for(int i=0; str[i] && i<255; i++) 
        buf[i] = str[i];
    st->ConOut->OutputString(st->ConOut, buf);
}



// Точка входа
EFI_STATUS __attribute__((ms_abi)) UefiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE* SystemTable
) {
    EFI_BOOT_SERVICES* BS = (EFI_BOOT_SERVICES*)SystemTable->BootServices;
    Print(SystemTable, "Bootloader started\r\n");

    // 1. Получаем протокол загруженного образа
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
    EFI_GUID loaded_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_STATUS status = ((EFI_STATUS(*)(EFI_HANDLE,EFI_GUID*,void**))BS->HandleProtocol)(
        ImageHandle, &loaded_guid, (void**)&LoadedImage
    );
    if(status) { Print(SystemTable, "Error: HandleProtocol 1\r\n"); return status; }

    // 2. Получаем файловую систему
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FS;
    EFI_GUID fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    status = ((EFI_STATUS(*)(EFI_HANDLE,EFI_GUID*,void**))BS->HandleProtocol)(
        LoadedImage->ParentHandle, &fs_guid, (void**)&FS
    );
    if(status) { Print(SystemTable, "Error: HandleProtocol 2\r\n"); return status; }

    // 3. Открываем корневой раздел
    EFI_FILE_PROTOCOL* Root;
    status = FS->OpenVolume(FS, (void**)&Root);
    if(status) { Print(SystemTable, "Error: OpenVolume\r\n"); return status; }
    Print(SystemTable, "Volume opened\r\n");

    // 4. Открываем файл ядра
    EFI_FILE_PROTOCOL* KernelFile;
    uint16_t kernel_path[] = u"EFI\\FOXOS\\kernel.bin";
    status = Root->Open(Root, (void**)&KernelFile, kernel_path, 0x01, 0);
    if(status) { Print(SystemTable, "Error: Open kernel.bin\r\n"); return status; }
    Print(SystemTable, "Kernel file opened\r\n");

    // 5. Получаем размер файла
    uint64_t info_size = 0;
    EFI_GUID file_info_guid = EFI_FILE_INFO_GUID;
    status = KernelFile->GetInfo(KernelFile, &file_info_guid, &info_size, 0);
    if(status != 0x8000000000000005) { // EFI_BUFFER_TOO_SMALL
        Print(SystemTable, "Error: GetInfo 1\r\n");
        return status;
    }

    // 6. Читаем информацию о файле
    void* file_info;
    EFI_PHYSICAL_ADDRESS mem_addr;
    status = BS->AllocatePages(1, 2, EFI_SIZE_TO_PAGES(info_size), &mem_addr); // AllocateAnyPages, EfiLoaderData
    if(status) { Print(SystemTable, "Error: AllocatePages 1\r\n"); return status; }
    file_info = (void*)mem_addr;
    
    status = KernelFile->GetInfo(KernelFile, &file_info_guid, &info_size, file_info);
    if(status) { Print(SystemTable, "Error: GetInfo 2\r\n"); return status; }
    uint64_t file_size = *(uint64_t*)(file_info + 32);
    Print(SystemTable, "Kernel size detected\r\n");

    // 7. Выделяем память под ядро
    EFI_PHYSICAL_ADDRESS kernel_addr;
    status = BS->AllocatePages(1, 1, EFI_SIZE_TO_PAGES(file_size), &kernel_addr); // EfiLoaderCode
    if(status) { Print(SystemTable, "Error: AllocatePages 2\r\n"); return status; }
    void* kernel_buffer = (void*)kernel_addr;
    Print(SystemTable, "Memory allocated\r\n");

    // 8. Читаем ядро в память
    status = KernelFile->Read(KernelFile, &file_size, kernel_buffer);
    if(status) { Print(SystemTable, "Error: Read kernel\r\n"); return status; }
    Print(SystemTable, "Kernel loaded\r\n");

    // Изменить вызов ExitBootServices
    uint64_t MapKey = 0;
    // Перед переходом на ядро
    Print(SystemTable, "Attempting ExitBootServices\r\n");
    status = BS->ExitBootServices(ImageHandle, MapKey);
    if (EFI_ERROR(status)) {
        Print(SystemTable, "ExitBootServices FAILED\r\n");
        return status;
    }

    // 10. Переход на ядро
    void (*KernelEntry)(void) = (void(*)(void))kernel_buffer;
    Print(SystemTable, "Jumping to kernel...\r\n");
    KernelEntry();

    // После загрузки ядра
    if(*(uint64_t*)kernel_buffer != 0x0000FFFFB8C30000) { // Сигнатура исполняемого кода
        Print(SystemTable, "Invalid kernel signature\r\n");
        return 0x8000000000000001; // EFI_LOAD_ERROR
    }
        
    return 0;
}