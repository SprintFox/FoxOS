#pragma once
#include <stddef.h>

// Базовые типы
typedef unsigned long long  UINT64;
typedef unsigned int        UINT32;
typedef unsigned short      UINT16;
typedef unsigned char       UINT8;
typedef UINT64              UINTN;
typedef UINT64              EFI_STATUS;
typedef void*               EFI_HANDLE;
typedef void*               EFI_EVENT;
typedef UINTN               EFI_TPL;
typedef wchar_t             CHAR16;

// Добавьте в начало файла после базовых типов
#define EFI_ERROR(Status) (((INTN)(Status)) < 0)
typedef long long INTN;

// GUID
typedef struct {
    UINT32  Data1;
    UINT16  Data2;
    UINT16  Data3;
    UINT8   Data4[8];
} EFI_GUID;

// Протоколы
#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    {0x5B1B31A1,0x9562,0x11d2, {0x8E,0x3F,0x00,0xA0,0xC9,0x69,0x72,0x3B}}

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    {0x964E5B22,0x6459,0x11D2, {0x8E,0x39,0x00,0xA0,0xC9,0x69,0x72,0x3B}}

// EFI_TABLE_HEADER
typedef struct {
    UINT64  Signature;
    UINT32  Revision;
    UINT32  HeaderSize;
    UINT32  CRC32;
    UINT32  Reserved;
} EFI_TABLE_HEADER;

// EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void* Reset;
    EFI_STATUS (*OutputString)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*, CHAR16*);
    // ... другие методы опущены
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

// EFI_SYSTEM_TABLE
typedef struct EFI_SYSTEM_TABLE {
    EFI_TABLE_HEADER                  Hdr;
    CHAR16*                           FirmwareVendor;
    UINT32                            FirmwareRevision;
    EFI_HANDLE                        ConsoleInHandle;
    void*                             ConIn;
    EFI_HANDLE                        ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*  ConOut;
    EFI_HANDLE                        StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*  StdErr;
    void*                             RuntimeServices;
    struct EFI_BOOT_SERVICES*         BootServices;
    // ... другие поля опущены
} EFI_SYSTEM_TABLE;

// EFI_BOOT_SERVICES
typedef struct EFI_BOOT_SERVICES {
    EFI_TABLE_HEADER Hdr;
    EFI_STATUS (*AllocatePool)(UINTN, UINTN, void**);
    EFI_STATUS (*FreePool)(void*);
    EFI_STATUS (*HandleProtocol)(EFI_HANDLE, EFI_GUID*, void**);
    // ... другие методы опущены
} EFI_BOOT_SERVICES;

typedef struct EFI_DEVICE_PATH {
    UINT8  Type;       // Тип узла устройства
    UINT8  SubType;    // Подтип
    UINT16 Length;     // Длина узла в байтах
    // ... (остальные поля зависят от конкретного типа устройства)
} EFI_DEVICE_PATH;

// EFI_LOADED_IMAGE_PROTOCOL
typedef struct {
    UINT32              Revision;
    EFI_HANDLE          ParentHandle;
    EFI_SYSTEM_TABLE*   SystemTable;
    EFI_HANDLE          DeviceHandle;
    EFI_DEVICE_PATH*    FilePath;         // Добавлено
    void*               Reserved;
    UINT32              LoadOptionsSize;  // Добавлено
    void*               LoadOptions;      // Добавлено
} EFI_LOADED_IMAGE_PROTOCOL;

// EFI_FILE_PROTOCOL
typedef struct EFI_FILE_PROTOCOL {
    UINT64      Revision;
    EFI_STATUS  (*Open)(struct EFI_FILE_PROTOCOL*, struct EFI_FILE_PROTOCOL**, CHAR16*, UINT64, UINT64);
    EFI_STATUS  (*Close)(struct EFI_FILE_PROTOCOL*);
    EFI_STATUS  (*Read)(struct EFI_FILE_PROTOCOL*, UINTN*, void*);
    EFI_STATUS  (*SetPosition)(struct EFI_FILE_PROTOCOL*, UINT64);
    EFI_STATUS  (*GetPosition)(struct EFI_FILE_PROTOCOL*, UINT64*);
    // ... другие методы опущены
} EFI_FILE_PROTOCOL;

// EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
typedef struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
    UINT64      Revision;
    EFI_STATUS  (*OpenVolume)(struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*, EFI_FILE_PROTOCOL**);
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

// Константы
#define EFI_SUCCESS 0
#define EFI_LOADER_DATA 1