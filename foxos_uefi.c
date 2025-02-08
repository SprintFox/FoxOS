// uefi_bootloader.c

#include <stdint.h>

#define NULL ((void*)0)

// Basic UEFI types
typedef uint64_t EFI_STATUS;
typedef void* EFI_HANDLE;
typedef void* EFI_EVENT;
typedef uint16_t CHAR16;

// GUID structure must be defined before it's used
typedef struct {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[8];
} EFI_GUID;

typedef struct {
    uint64_t Signature;
    uint32_t Revision;
    uint32_t HeaderSize;
    uint32_t CRC32;
    uint32_t Reserved;
} EFI_TABLE_HEADER;

// Forward declarations
typedef struct _EFI_BOOT_SERVICES EFI_BOOT_SERVICES;
typedef struct _EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void* Reset;
    EFI_STATUS (*OutputString)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*, CHAR16*);
    // ... другие методы опущены
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct {
    EFI_TABLE_HEADER Hdr;
    CHAR16* FirmwareVendor;
    uint32_t FirmwareRevision;
    void* ConsoleInHandle;
    void* ConIn;
    void* ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*  ConOut;
    void* StandardErrorHandle;
    void* StdErr;
    void* RuntimeServices;
    EFI_BOOT_SERVICES* BootServices;
} EFI_SYSTEM_TABLE;

struct _EFI_BOOT_SERVICES {
    EFI_TABLE_HEADER Hdr;          // Размер 24 байта (предполагая 64-битную систему)
    
    // Пропуск первых 5 функций после Hdr (RaiseTPL, RestoreTPL, AllocatePages, FreePages, GetMemoryMap)
    uint64_t _pad_before_allocate_pool[5];
    
    // 6-я функция: AllocatePool
    EFI_STATUS (*AllocatePool)(uint32_t PoolType, uint64_t Size, void** Buffer);
    
    // Пропуск следующих 10 функций (FreePool, SetWatchdogTimer, ConnectController, ..., до LocateProtocol)
    uint64_t _pad_before_locate_protocol[10];
    
    // 16-я функция: LocateProtocol
    EFI_STATUS (*LocateProtocol)(EFI_GUID* Protocol, void* Registration, void** Interface);
    
    // ... Дополнительные паддинги, если требуется пропустить функции после LocateProtocol
};

typedef struct {
    uint64_t Revision;
    EFI_STATUS (*OpenVolume)(void* This, EFI_FILE_PROTOCOL** Root);
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

struct _EFI_FILE_PROTOCOL {
    uint64_t Revision;
    EFI_STATUS (*Open)(void* This, EFI_FILE_PROTOCOL** NewHandle, CHAR16* FileName, 
                      uint64_t OpenMode, uint64_t Attributes);
    EFI_STATUS (*Close)(void* This);
    EFI_STATUS (*Read)(void* This, uint64_t* BufferSize, void* Buffer);
    EFI_STATUS (*GetInfo)(void* This, EFI_GUID* InformationType, uint64_t* BufferSize, void* Buffer);
};

// Static GUID definitions
static EFI_GUID EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID = {
    0x0964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}
};

static EFI_GUID EFI_FILE_INFO_GUID = {
    0x09576e92, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}
};

// Точка входа UEFI
EFI_STATUS __attribute__((ms_abi)) UefiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    unsigned short str[] = u"Hello World!\r\n";
    SystemTable->ConOut->OutputString(SystemTable->ConOut, str);
    EFI_STATUS status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
    EFI_FILE_PROTOCOL* root;
    EFI_FILE_PROTOCOL* kernel_file;

    SystemTable->ConOut->OutputString(SystemTable->ConOut, u"Starting FoxOS UEFI\r\n");
    
    // Получаем протокол файловой системы
    status = SystemTable->BootServices->LocateProtocol(&EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID, 0, (void**)&fs);
    if(status) return status;

    SystemTable->ConOut->OutputString(SystemTable->ConOut, u"Loaded Simple File System Protocol\r\n");
    
    // Открываем корневой раздел
    status = fs->OpenVolume(fs, &root);
    if(status) return status;


    SystemTable->ConOut->OutputString(SystemTable->ConOut, u"Opened Root Volume\r\n");
    
    // Открываем файл ядра
    CHAR16 kernel_path[] = u"/EFI/FOXOS/kernel.bin";
    status = root->Open(root, &kernel_file, kernel_path, 1, 0);
    if(status) return status;
    

    SystemTable->ConOut->OutputString(SystemTable->ConOut, u"Opened Kernel File\r\n");

    // Читаем файл
    uint64_t file_size;
    kernel_file->GetInfo(kernel_file, &EFI_FILE_INFO_GUID, &file_size, NULL);
    void* kernel_buffer;
    SystemTable->BootServices->AllocatePool(0, file_size, &kernel_buffer);
    kernel_file->Read(kernel_file, &file_size, kernel_buffer);

    SystemTable->ConOut->OutputString(SystemTable->ConOut, u"Read Kernel File\r\n");
    
    // Загружаем ядро в память
    void (*kernel_entry)(void) = (void(*)(void))kernel_buffer;
    kernel_entry();
    
    return 0;
}