# GRUB対応 LiteCore Makefile (C言語ベース)
KERNEL_DIR = kernel
BUILD_DIR = bin

KERNEL_C_SRCS = kernel/lib/string.c kernel/lib/serial.c kernel/lib/debug.c kernel/lib/stdio.c mem/memory.c kernel/main.c kernel/vga/console.c kernel/util/scheduler.c kernel/util/gdt/gdt.c kernel/util/idt/idt.c kernel/util/interrupt/interrupt_handler.c kernel/util/interrupt/interrupt.c
OTHER_ASM_SRCS = kernel/util/gdt/gdt_asm.asm kernel/util/idt/idt_asm.asm kernel/util/interrupt/interrupt_asm.asm
KERNEL_OBJS = bin/string.o bin/serial.o bin/debug.o bin/stdio.o bin/memory.o bin/main.o bin/console.o bin/scheduler.o bin/gdt.o bin/idt.o bin/interrupt_handler.o bin/interrupt.o bin/multiboot.o bin/gdt_asm.o bin/idt_asm.o bin/interrupt_asm.o

KERNEL_BIN = $(BUILD_DIR)/kernel.bin
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
ISO = $(BUILD_DIR)/litecore.iso

VERSION = $(shell cat version.txt 2>/dev/null || echo "dev")
CFLAGS = -m64 -ffreestanding -fno-pic -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Iinclude -Iinclude/lib -Iinclude/util -Iinclude/vga -Ikernel -Ikernel/lib -Ikernel/util -Ikernel/util/gdt -Ikernel/util/idt -Ikernel/util/interrupt -Ikernel/vga -Imem -g -O0 -DKERNEL_VERSION=\"$(VERSION)\"

all: $(ISO)

kernel: $(KERNEL_BIN)

$(BUILD_DIR)/gdt_asm.o: kernel/util/gdt/gdt_asm.asm
	mkdir -p $(BUILD_DIR)
	nasm -f elf64 -g -F dwarf -o $@ $< -w-gnu-stack

$(BUILD_DIR)/idt_asm.o: kernel/util/idt/idt_asm.asm
	mkdir -p $(BUILD_DIR)
	nasm -f elf64 -g -F dwarf -o $@ $< -w-gnu-stack

$(BUILD_DIR)/interrupt_asm.o: kernel/util/interrupt/interrupt_asm.asm
	mkdir -p $(BUILD_DIR)
	nasm -f elf64 -g -F dwarf -o $@ $< -w-gnu-stack

$(BUILD_DIR)/string.o: kernel/lib/string.c include/lib/string.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/serial.o: kernel/lib/serial.c include/lib/serial.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/debug.o: kernel/lib/debug.c include/lib/debug.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/stdio.o: kernel/lib/stdio.c 
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/memory.o: mem/memory.c include/memory.h include/vga/console.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: kernel/main.c include/multiboot.h include/vga/console.h include/system.h include/memory.h include/scheduler.h include/config.h include/lib/serial.h include/lib/debug.h include/lib/early_debug.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/console.o: kernel/vga/console.c include/lib/string.h include/vga/console.h include/config.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/scheduler.o: kernel/util/scheduler.c include/scheduler.h include/vga/console.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/gdt.o: kernel/util/gdt/gdt.c include/util/gdt.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/idt.o: kernel/util/idt/idt.c include/util/idt.h include/util/interrupt.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/interrupt_handler.o: kernel/util/interrupt/interrupt_handler.c include/util/interrupt_handler.h include/vga/console.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/interrupt.o: kernel/util/interrupt/interrupt.c include/util/interrupt.h include/vga/console.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@


# Multibootヘッダーの明示的なコンパイル
$(BUILD_DIR)/multiboot.o: boot/multiboot.asm
	mkdir -p $(BUILD_DIR)
	nasm -f elf64 -g -o $@ $< -w-gnu-stack

$(KERNEL_ELF): $(KERNEL_OBJS)
	ld -m elf_x86_64 -T $(KERNEL_DIR)/linker.ld -o $(KERNEL_ELF) --build-id=none -g $^ -z noexecstack
	@echo "\033[0;32mKernel ELF size: $$(wc -c < $@) bytes\033[0m"

$(KERNEL_BIN): $(KERNEL_ELF)
	objcopy -O binary $(KERNEL_ELF) $(KERNEL_BIN)
	@echo "\033[0;32mKernel binary size: $$(wc -c < $@) bytes\033[0m"

$(ISO): $(KERNEL_ELF)
	mkdir -p $(BUILD_DIR)/iso/boot/grub
	cp $(KERNEL_ELF) $(BUILD_DIR)/iso/boot/
	cp grub/grub.cfg $(BUILD_DIR)/iso/boot/grub/
	grub-mkrescue -o $(ISO) $(BUILD_DIR)/iso
	@echo "\033[0;32mGRUB ISO created: $(ISO)\033[0m"

run: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -monitor stdio -cpu qemu64 -no-reboot -no-shutdown

run-debug: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -monitor stdio -cpu qemu64 -no-reboot -no-shutdown -d int,guest_errors -D qemu.log

debug: $(ISO) clean-qemu
	@echo "Starting QEMU with GDB server on port 1234..."
	qemu-system-x86_64 -cdrom $(ISO) -s -S -cpu qemu64 -no-reboot -no-shutdown &
	@sleep 1
	@echo "Starting GDB and connecting to QEMU..."
	gdb -ex "file $(KERNEL_ELF)" -ex "target remote localhost:1234" -ex "set disassembly-flavor intel" -ex "layout asm" -ex "break *0x1000" -ex "continue"

clean-qemu:
	@echo "Killing any existing QEMU processes..."
	@-pkill -9 qemu-system-x86_64 2>/dev/null || true
	@sleep 1

clean:
	rm -rf $(BUILD_DIR)

memory-map: $(KERNEL_ELF)
	@echo "======== Kernel memory map ========"
	@objdump -h $(KERNEL_ELF)
	@echo "\n======== Kernel symbol table ========"
	@nm $(KERNEL_ELF) | sort
	@echo "\n======== Entry point & sections ========"
	-@readelf -h $(KERNEL_ELF) | grep "Entry point"
	@readelf -S $(KERNEL_ELF)
	@echo "\n======== Disassembly of start ========"
	@objdump -d -j .text.boot $(KERNEL_ELF)

.PHONY: all kernel run run-debug debug clean clean-qemu memory-map
