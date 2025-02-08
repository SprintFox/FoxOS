ENTRY(_start)
SECTIONS
{
    . = 0x100000;
    .magic ALIGN(1) : {
        *(.magic)
    }
    .text ALIGN(1) : {
        *(.text)
    }
    .rodata ALIGN(1) : {
        *(.rodata)
    }
    .data ALIGN(1) : {
        *(.data)
    }
    .bss ALIGN(1) : {
        *(.bss)
    }
} 