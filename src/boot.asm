[BITS 16]
ORG 0x7C00

; Загрузчик первой стадии
start:
    ; Настройка стека
    cli
    xor ax, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Загрузка второй стадии
    mov ah, 0x02          ; Функция чтения секторов
    mov al, 64            ; Количество секторов (16 КБ / 512 = 16)
    mov ch, 0             ; Цилиндр 0
    mov cl, 2             ; Начиная со второго сектора
    mov dh, 0             ; Головка 0
    mov bx, 0x7E00        ; Адрес загрузки (сразу после первого загрузчика)
    int 0x13              ; BIOS прерывание для чтения диска
    
    jc disk_error         ; Если произошла ошибка

    mov si, stage2_msg
    call print_string

    ; Переход на вторую стадию
    jmp 0x0000:0x7E00

disk_error:
    mov si, error_msg
    call print_string
    jmp $

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

error_msg db "Error loading second stage!", 0
stage2_msg db "Starting stage 2...", 0

times 510-($-$$) db 0
dw 0xAA55