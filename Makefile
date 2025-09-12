# GRUB対応 LiteCore Makefile (C言語ベース)
KERNEL_DIR = kernel
BUILD_DIR = bin

KERNEL_C_SRCS = kernel/main.c kernel/console.c
OTHER_ASM_SRCS = 
KERNEL_OBJS = bin/main.o bin/console.o

KERNEL_BIN = $(BUILD_DIR)/kernel.bin
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
ISO = $(BUILD_DIR)/litecore.iso

VERSION = $(shell cat version.txt 2>/dev/null || echo "dev")
CFLAGS = -m64 -ffreestanding -fno-pic -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Iinclude -Ikernel -g -O0 -DKERNEL_VERSION=\"$(VERSION)\"

all: $(ISO)

kernel: $(KERNEL_BIN)

$(BUILD_DIR)/main.o: kernel/main.c include/multiboot.h include/console.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/console.o: kernel/console.c include/console.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@


$(KERNEL_BIN): $(KERNEL_OBJS)
	ld -m elf_x86_64 -T $(KERNEL_DIR)/linker.ld -o $(KERNEL_ELF) --build-id=none -g $^ -z noexecstack
	objcopy -O binary $(KERNEL_ELF) $@
	@echo "\033[0;32mKernel size: $$(wc -c < $@) bytes\033[0m"

$(ISO): $(KERNEL_BIN)
	mkdir -p $(BUILD_DIR)/iso/boot/grub
	cp $(KERNEL_BIN) $(BUILD_DIR)/iso/boot/
	cp grub/grub.cfg $(BUILD_DIR)/iso/boot/grub/
	grub-mkrescue -o $(ISO) $(BUILD_DIR)/iso
	@echo "\033[0;32mGRUB ISO created: $(ISO)\033[0m"

run: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -monitor stdio -cpu qemu64 -no-reboot -no-shutdown

run-debug: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -monitor stdio -cpu qemu64 -no-reboot -no-shutdown -d int,cpu_reset,exec,in_asm,page -D qemu.log

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
