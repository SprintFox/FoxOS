# Makefile

BOOT_SRC = src/boot.asm
STAGE2_SRC = src/stage2.asm
KERNEL_SRC = src/kernel.c
VGA_SRC = src/vga.c
KEYBOARD_SRC = src/keyboard.c
TERMINAL_SRC = src/terminal.c
FS_SRC = src/fs.c

BOOT_BIN = bin/boot.bin
STAGE2_BIN = bin/stage2.bin
KERNEL_BIN = bin/kernel.bin
KERNEL_OBJ = bin/kernel.o
VGA_OBJ = bin/vga.o
KEYBOARD_OBJ = bin/keyboard.o
TERMINAL_OBJ = bin/terminal.o
FS_OBJ = bin/fs.o

LD = x86_64-elf-ld
CC = x86_64-elf-gcc
NASM = nasm

CFLAGS = -m64 -ffreestanding -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
         -O2 -nostdlib -nostdinc -fno-pie -no-pie -mcmodel=kernel \
         -fno-stack-protector -fno-exceptions -I src

LDFLAGS = -n -T linker.ld --oformat binary -nostdlib

# UEFI specific targets
UEFI_BOOTLOADER = build/bootx64.efi
UEFI_ESP = build/esp

UEFI_CFLAGS = -I/usr/local/include/efi \
              -I/usr/local/include/efi/x86_64 \
              -fno-stack-protector -fpic \
              -fshort-wchar -mno-red-zone \
              -DEFI_FUNCTION_WRAPPER

$(UEFI_BOOTLOADER): src/uefi_boot.c
	mkdir -p build/
	$(CC) $(UEFI_CFLAGS) -c -o build/uefi_boot.o src/uefi_boot.c
	$(LD) -nostdlib -znocombreloc -shared -Bsymbolic \
		-L/usr/local/lib -T/usr/local/lib/elf_x86_64_efi.lds \
		/usr/local/lib/crt0-efi-x86_64.o build/uefi_boot.o \
		-o $(UEFI_BOOTLOADER) -lgnuefi -lefi

bootloader: $(UEFI_BOOTLOADER)

all: bin os-image

bin:
	mkdir -p bin

# os-image: $(BOOT_BIN) $(STAGE2_BIN) $(KERNEL_BIN)
# 	cat $(BOOT_BIN) $(STAGE2_BIN) $(KERNEL_BIN) > bin/os-image

os-image: $(BOOT_BIN) $(STAGE2_BIN) $(KERNEL_BIN)
	dd if=/dev/zero of=bin/os-image.bin bs=512 count=2880
	dd if=$(BOOT_BIN) of=bin/os-image.bin conv=notrunc
	dd if=$(STAGE2_BIN) of=bin/os-image.bin seek=1 conv=notrunc
	dd if=$(KERNEL_BIN) of=bin/os-image.bin seek=2048 conv=notrunc
	
$(BOOT_BIN): $(BOOT_SRC)
	$(NASM) -f bin $(BOOT_SRC) -o $(BOOT_BIN)

$(STAGE2_BIN): $(STAGE2_SRC)
	$(NASM) -f bin $(STAGE2_SRC) -o $(STAGE2_BIN)

$(KERNEL_OBJ): $(KERNEL_SRC)
	$(CC) $(CFLAGS) -c $(KERNEL_SRC) -o $(KERNEL_OBJ)

$(VGA_OBJ): $(VGA_SRC)
	$(CC) $(CFLAGS) -c $(VGA_SRC) -o $(VGA_OBJ)

$(KEYBOARD_OBJ): $(KEYBOARD_SRC)
	$(CC) $(CFLAGS) -c $(KEYBOARD_SRC) -o $(KEYBOARD_OBJ)

$(TERMINAL_OBJ): $(TERMINAL_SRC)
	$(CC) $(CFLAGS) -c $(TERMINAL_SRC) -o $(TERMINAL_OBJ)

$(FS_OBJ): $(FS_SRC)
	$(CC) $(CFLAGS) -c $(FS_SRC) -o $(FS_OBJ)

$(KERNEL_BIN): $(KERNEL_OBJ) $(VGA_OBJ) $(KEYBOARD_OBJ) $(TERMINAL_OBJ) $(FS_OBJ)
	$(LD) $(LDFLAGS) -o $(KERNEL_BIN) $(KERNEL_OBJ) $(VGA_OBJ) $(KEYBOARD_OBJ) $(TERMINAL_OBJ) $(FS_OBJ)

clean:
	rm -f bin/*

run: os-image
	qemu-system-x86_64 -drive format=raw,file=bin/os-image.bin,index=0 -no-reboot -no-shutdown