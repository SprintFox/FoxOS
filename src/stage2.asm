[BITS 16]
ORG 0x7E00

start:
    mov si, stage2_loaded_msg
    call print_string

    ; Загружаем ядро с диска
    mov ah, 0x42          ; Extended Read
    mov dl, 0x80         ; First hard drive
    mov si, dap          ; Disk Address Packet
    int 0x13
    jc disk_error

    mov si, kernel_loaded_msg
    call print_string

    ; Переход в защищённый режим
    cli
    lgdt [gdt_descriptor]

    ; Включаем A20 линию
    in al, 0x92
    or al, 2
    out 0x92, al
    
    ; Включаем защищенный режим
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    ; Прыжок для очистки конвейера
    jmp 0x08:protected_mode

print_string:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print_string
.done:
    mov al, 13
    int 0x10
    mov al, 10
    int 0x10
    ret

stage2_loaded_msg db "Stage 2 loaded!", 0
kernel_loaded_msg db "Kernel loaded!", 0
disk_error_msg db "Error loading kernel!", 0

; Disk Address Packet
align 4
dap:
    db 0x10      ; размер DAP (16 байт)
    db 0         ; всегда 0
    dw 32        ; количество секторов для чтения (16 КБ)
    dw 0x0000    ; смещение
    dw 0x1000    ; сегмент (0x1000 * 16 = 0x10000)
    dq 2048      ; номер начального сектора (LBA)

; GDT
gdt:
    ; Нулевой дескриптор
    dq 0

    ; Дескриптор кода для 32-битного режима (сегмент 0x08)
    dw 0xFFFF       ; Лимит (младшие 16 бит)
    dw 0x0000       ; База (младшие 16 бит)
    db 0x00         ; База (следующие 8 бит)
    db 10011010b    ; Флаги доступа (исполняемый, привилегия 0, доступен)
    db 11001111b    ; Флаги (4Кб гранулярность, 32-битный сегмент)
    db 0x00         ; База (старшие 8 бит)

    ; Дескриптор данных для 32-битного режима (сегмент 0x10)
    dw 0xFFFF       ; Лимит (младшие 16 бит)
    dw 0x0000       ; База (младшие 16 бит)
    db 0x00         ; База (следующие 8 бит)
    db 10010010b    ; Флаги доступа (чтение/запись, привилегия 0, доступен)
    db 11001111b    ; Флаги (4Кб гранулярность, 32-битный сегмент)
    db 0x00         ; База (старшие 8 бит)

    ; Дескриптор кода для 64-битного режима (сегмент 0x18)
    dw 0x0000       ; Лимит (младшие 16 бит) — игнорируется
    dw 0x0000       ; База (младшие 16 бит) — игнорируется
    db 0x00         ; База (следующие 8 бит) — игнорируется
    db 10011010b    ; Флаги доступа (исполняемый, привилегия 0, доступен)
    db 10101111b    ; Флаги (4Кб гранулярность, Long mode)
    db 0x00         ; База (старшие 8 бит) — игнорируется


gdt_descriptor:
    dw $ - gdt - 1
    dd gdt

[BITS 32]
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    mov esp, 0x90000

    ; Заполняем видеопамять
    mov edi, 0xB8000
    mov eax, 0x07200720
    mov ecx, 80 * 25
    rep stosd

    ; Индикация входа в защищенный режим
    mov byte [0xB8000], 'P'
    mov byte [0xB8001], 0x4F
    mov byte [0xB8002], 'M'
    mov byte [0xB8003], 0x4F

    ; Копируем ядро из временного адреса в финальный
    mov esi, 0x10000      ; Исходный адрес (куда мы загрузили ядро)
    mov edi, 0x100000     ; Целевой адрес (куда нужно ядро)
    mov ecx, 8192         ; Количество двойных слов (16 КБ / 4)
    rep movsd             ; Копируем по 4 байта за раз

    ; Подготовка таблиц страниц
    ; Очищаем PML4
    mov edi, 0x9000
    xor eax, eax
    mov ecx, 4096
    rep stosd

    ; Настраиваем таблицы страниц
    mov dword [0x9000], 0xA000 | 3    ; PML4[0] -> PDPT
    mov dword [0xA000], 0xB000 | 3    ; PDPT[0] -> PD
    
    ; Настраиваем записи в PD для отображения первых 2MB
    mov edi, 0xB000
    mov eax, 0x0 | 0x83                ; Present + Write + Huge Page (2MB)
    mov ecx, 512                       ; Map 1GB (512 entries * 2MB)
.map_pd:
    mov [edi], eax
    add eax, 0x200000                  ; Next 2MB
    add edi, 8                         ; Next entry
    loop .map_pd

    ; Загружаем PML4
    mov eax, 0x9000
    mov cr3, eax

    ; Включение PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Включение Long Mode через EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Включение страничной памяти
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; Переход в 64-битный режим
    jmp 0x18:long_mode

disk_error:
    mov si, disk_error_msg
    call print_string
    jmp $

[BITS 64]
long_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov byte [0xB80A0], '6'
    mov byte [0xB80A1], 0x5F
    mov byte [0xB80A2], '4'
    mov byte [0xB80A3], 0x5F

    mov ecx, 0xC0000080  ; Индекс EFER
    rdmsr
    ; Проверьте, установлен ли 10-й бит (LMA)
    test eax, (1 << 10)
    jz not_in_long_mode

    mov rax, 0x1234567812345678
    mov [0xB8000], rax  ; Отобразить часть значения для проверки


    ; Индикация успешного перехода в long mode
    mov byte [0xB80A4], 'L'
    mov byte [0xB80A5], 0x5F
    mov byte [0xB80A6], 'M'
    mov byte [0xB80A7], 0x5F

    ; Небольшая задержка для стабилизации
    mov ecx, 0x10000000
.delay:
    loop .delay

    mov byte [0xB8140], 'J'
    mov byte [0xB8141], 0xEF

    ; Вывод первых 16 байт ядра
    mov rsi, 0x100000      ; Адрес ядра
    mov rdi, 0xB8160       ; Позиция на экране
    mov rcx, 16            ; Количество байт для вывода

.print_kernel_bytes:
    mov al, [rsi]          ; Загружаем байт из ядра
    mov ah, 0x0F           ; Атрибут (белый на черном)
    mov [rdi], ax          ; Выводим байт и атрибут
    inc rsi                ; Следующий байт ядра
    add rdi, 2             ; Следующая позиция на экране
    loop .print_kernel_bytes

    ; Переход на ядро
    mov rax, 0x100000
    jmp rax

not_in_long_mode:
    mov byte [0xB80A8], 'E'
    mov byte [0xB80A9], 0x4F
    mov byte [0xB80AA], 'R'
    mov byte [0xB80AB], 0x4F

times 32768-($-$$) db 0
