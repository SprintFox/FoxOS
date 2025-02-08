// Magic number to identify kernel
__attribute__((section(".magic"))) unsigned long long kernel_magic = 0x4B45524E454C4F53; // "KERNELOS" in ASCII

#include "vga.h"
#include "keyboard.h"
#include "terminal.h"
#include "fs.h"

void _start(void) {
    // Инициализация VGA
    vga_init();
    vga_puts("FoxOS starting... ");
    vga_puts("OK\n");

    // // Инициализация PCI
    // vga_puts("Initializing PCI... ");
    // pci_init();
    // vga_puts("OK\n");
    
    // // Инициализация AHCI
    // vga_puts("Initializing AHCI... ");
    // if (ahci_init() != 0) {
    //     vga_puts("Failed!\n");
    // } else {
    //     vga_puts("OK\n");
    // }
    
    // Инициализация файловой системы
    vga_puts("Initializing filesystem... ");
    fs_init();
    vga_puts("OK\n");
    
    // Инициализация клавиатуры
    vga_puts("Initializing keyboard... ");
    keyboard_init();
    vga_puts("OK\n");
    
    vga_puts("\nWelcome to FoxOS!\n");
    vga_puts("Type 'help' for list of commands.\n\n");
    
    // Запуск терминала
    terminal_init();
    
    while(1) {
        terminal_run();
    };
}