#include "Uefi.h"

// Точка входа UEFI приложения
EFI_STATUS __attribute__((ms_abi)) UefiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE* SystemTable
) {
    unsigned short str[] = u"Hello World!\r\n";
    SystemTable->ConOut->OutputString(SystemTable->ConOut, str);

    EFI_BOOT_SERVICES* BS = SystemTable->BootServices;

    // Получаем протокол загруженного изображения
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage = NULL;
    EFI_GUID lipGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    
    EFI_STATUS status = BS->HandleProtocol(
        ImageHandle, 
        &lipGuid, 
        (void**)&LoadedImage
    );
    
    if (EFI_ERROR(status)) {
        CHAR16* errorStr = (CHAR16*)u"Error getting LoadedImageProtocol: \r\n", status;
        SystemTable->ConOut->OutputString(SystemTable->ConOut, errorStr);
        return status;
    }



    SystemTable->ConOut->OutputString(SystemTable->ConOut, u"Got LoadedImageProtocol\r\n");

    // Получаем протокол файловой системы
    EFI_GUID fsGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem = NULL;
    status = BS->HandleProtocol(LoadedImage->DeviceHandle, &fsGuid, (void**)&FileSystem);
    if (status != 0) {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, u"Error getting FileSystemProtocol\r\n");
        return status;
    }

    // Открываем корневой раздел
    EFI_FILE_PROTOCOL *Root = NULL;
    status = FileSystem->OpenVolume(FileSystem, &Root);
    if (status != 0) {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, u"Error opening volume\r\n");
        return status;
    }

    // Открываем файл ядра
    EFI_FILE_PROTOCOL *KernelFile = NULL;
    unsigned short kernelPath[] = u"\\EFI\\FOXOS\\kernel.bin";
    status = Root->Open(Root, &KernelFile, kernelPath, 0x01 /* EFI_FILE_MODE_READ */, 0);
    if (status != 0) {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, u"Error opening kernel file\r\n");
        Root->Close(Root);
        return status;
    }

    // Получаем размер файла
    UINT64 fileSize;
    KernelFile->SetPosition(KernelFile, 0xFFFFFFFFFFFFFFFFULL); // Переход в конец файла
    KernelFile->GetPosition(KernelFile, &fileSize);
    KernelFile->SetPosition(KernelFile, 0); // Сброс позиции

    

    // Выделяем память под ядро
    void *kernelBuffer;
    status = BS->AllocatePool(1 /* EfiLoaderData */, (UINTN)fileSize, &kernelBuffer);
    if (status != 0) {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, u"Memory allocation failed\r\n");
        KernelFile->Close(KernelFile);
        Root->Close(Root);
        return status;
    }

    // Читаем файл в память
    UINTN readSize = (UINTN)fileSize;
    status = KernelFile->Read(KernelFile, &readSize, kernelBuffer);
    if (status != 0 || readSize != fileSize) {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, u"Error reading kernel file\r\n");
        BS->FreePool(kernelBuffer);
        KernelFile->Close(KernelFile);
        Root->Close(Root);
        return status;
    }

    // Закрываем файлы
    KernelFile->Close(KernelFile);
    Root->Close(Root);

    SystemTable->ConOut->OutputString(SystemTable->ConOut, u"Kernel loaded successfully!\r\n");

    // Здесь должна быть логика передачи управления ядру
    // Например: ((void (*)())kernelBuffer)();

    return 0;
}