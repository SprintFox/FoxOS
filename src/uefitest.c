// Определение базовых типов UEFI
typedef unsigned long long EFI_STATUS;
typedef void* EFI_HANDLE;
typedef void* EFI_EVENT;
typedef unsigned long long EFI_TPL;

// Структура EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
typedef struct {
    void* Reset;
    EFI_STATUS (*OutputString)(void* This, unsigned short* String);
    void* TestString;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

// Структура EFI_SYSTEM_TABLE
typedef struct {
    char _pad[60];
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut;
} EFI_SYSTEM_TABLE;

// Точка входа UEFI приложения
EFI_STATUS __attribute__((ms_abi)) UefiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE* SystemTable
) {
    // Строка в формате UTF-16
    unsigned short str[] = u"Hello World!\r\n";

    // Вывод строки через консольный вывод
    SystemTable->ConOut->OutputString(SystemTable->ConOut, str);

    return 0;
}