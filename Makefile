CC         = gcc
LD         = ld
NASM       = nasm
QEMU       = qemu-system-x86_64
OBJCOPY    = objcopy
SHELL      = /bin/bash

EDK2_DIR   = uefi
EDK_SETUP  = $(EDK2_DIR)/edksetup.sh
BOOT_PKG   = boot/LiteCoreBootManager.dsc
EDK2_BUILD = $(EDK2_DIR)/bin/boot/DEBUG_GCC5/X64/LiteCoreBootManager.efi

SRC_DIR    = src
SRC_BOOT   = boot
SRC_KERNEL = $(SRC_DIR)/kernel
INCLUDE    = $(SRC_DIR)/include
OUT_DIR    = bin
K_OUT_DIR  = $(OUT_DIR)/kernel
B_OUT_DIR  = $(OUT_DIR)/boot
IMG_OUT_DIR = $(OUT_DIR)

CFLAGS     = -O2 -Wimplicit-function-declaration -Wunused-but-set-variable -ffreestanding -m64 -c -Wall -Wextra -I$(INCLUDE) -mcmodel=large -mno-red-zone -fno-pic
LDFLAGS    = -m elf_x86_64 -z max-page-size=0x1000

NFLAGS     = -f bin
QEMU_FLAGS = -serial stdio -display none -monitor none -device qemu-xhci,id=xhci \
             -device usb-kbd,bus=xhci.0 \
             -bios /usr/share/ovmf/OVMF.fd -d int -D qemu.log --no-reboot
QEMU_VGA   = -display curses -device qemu-xhci,id=xhci -device usb-kbd,bus=xhci.0 -bios /usr/share/ovmf/OVMF.fd
CONSOLE    = -display curses

SOURCES    = $(shell find $(SRC_KERNEL) -name "*.c")
ASM_SOURCES = $(shell find $(SRC_KERNEL) -name "*.asm")
OBJECTS    = $(shell printf "%s\n" $(patsubst $(SRC_KERNEL)/%.c, $(K_OUT_DIR)/%.o, $(SOURCES)) $(patsubst $(SRC_KERNEL)/%.asm, $(K_OUT_DIR)/%.o, $(ASM_SOURCES)) | sort -u)

BOOTX64    = $(B_OUT_DIR)/BOOTX64.EFI
KERNEL_ELF = $(K_OUT_DIR)/kernel.elf
KERNEL     = $(K_OUT_DIR)/kernel.bin
ESP_IMG    = $(IMG_OUT_DIR)/LiteCore.img
EXT2_IMG   = $(IMG_OUT_DIR)/fs.img
LINKER     = $(SRC_DIR)/kernel.ld
ESP_DIR    = esp

.PHONY: all run run-console run-vga clean bootloader kernel ext2
.DEFAULT_GOAL := all

all: bootloader kernel $(ESP_IMG) $(EXT2_IMG)

$(K_OUT_DIR):
	@mkdir -p $(K_OUT_DIR)

bootloader: $(BOOTX64)

$(BOOTX64): $(SRC_BOOT)/boot.c $(SRC_BOOT)/Boot.inf $(BOOT_PKG)
	@echo "Building LiteCoreBootManager..."
	@cd $(EDK2_DIR) && . edksetup.sh && build -p ../$(BOOT_PKG) -a X64 -t GCC5 -b DEBUG -q
	@mkdir -p $(B_OUT_DIR)
	@cp $(EDK2_BUILD) $(BOOTX64)
	@echo "created: $(BOOTX64)"

kernel: $(KERNEL)

$(KERNEL_ELF): $(K_OUT_DIR) $(OBJECTS) $(LINKER)
	$(LD) $(LDFLAGS) -T $(LINKER) $(OBJECTS) -o $@

$(KERNEL): $(KERNEL_ELF)
	@$(OBJCOPY) -O binary $< $@
	@echo "Kernel size: $$(wc -c < $@) bytes"

$(K_OUT_DIR)/%.o: $(SRC_KERNEL)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(K_OUT_DIR)/%.o: $(SRC_KERNEL)/%.asm
	@mkdir -p $(dir $@)
	@$(NASM) -f elf64 $< -o $@

$(ESP_IMG): $(BOOTX64) $(KERNEL)
	@echo "Creating UEFI ESP image..."
	@rm -f $(ESP_IMG)
	@dd if=/dev/zero of=$(ESP_IMG) bs=1M count=64 2>/dev/null
	@mkfs.vfat -F 32 $(ESP_IMG) >/dev/null 2>&1
	@mkdir -p $(OUT_DIR)/.mnt
	@sudo mount -o loop $(ESP_IMG) $(OUT_DIR)/.mnt
	@sudo mkdir -p $(OUT_DIR)/.mnt/EFI/BOOT
	@sudo cp $(BOOTX64) $(OUT_DIR)/.mnt/EFI/BOOT/BOOTX64.EFI
	@sudo cp $(KERNEL) $(OUT_DIR)/.mnt/kernel.bin
	@sudo umount $(OUT_DIR)/.mnt
	@rmdir $(OUT_DIR)/.mnt
	@echo "ESP image created: $(ESP_IMG)"

$(EXT2_IMG): $(KERNEL)
	@echo "Creating ext2 filesystem image..."
	@mkdir -p bin/fs_tmp
	@cp -r bin/kernel/*.bin bin/fs_tmp/kernel/ 2>/dev/null || true
	@cp -r bin/kernel/*.elf bin/fs_tmp/kernel/ 2>/dev/null || true
	@cp -r bin/boot/*.EFI bin/fs_tmp/boot/ 2>/dev/null || true
	@cp -r bin/LiteCore.img bin/fs_tmp 2>/dev/null || true nt/ || true
	@python3 tools/mk_ext2_image.py $(EXT2_IMG) 2048 bin/fs_tmp
	@rm -rf bin/fs_tmp/fs_content
	@echo "ext2 image created: $(EXT2_IMG)"

run: $(ESP_IMG) $(EXT2_IMG)
	$(QEMU) $(QEMU_FLAGS) --drive file=$(ESP_IMG),format=raw -drive file=$(EXT2_IMG),format=raw,if=ide $(QEMU_USB)

run-console: $(ESP_IMG) $(EXT2_IMG)
	$(QEMU) -bios /usr/share/ovmf/OVMF.fd $(CONSOLE) $(QEMU_USB) -drive file=$(ESP_IMG),format=raw -drive file=$(EXT2_IMG),format=raw,if=ide

run-vga: $(ESP_IMG) $(EXT2_IMG)
	$(QEMU) -bios /usr/share/ovmf/OVMF.fd $(QEMU_VGA) -drive file=$(ESP_IMG),format=raw -drive file=$(EXT2_IMG),format=raw,if=ide

clean:
	rm -rf $(OUT_DIR) $(ESP_DIR)
	cd $(EDK2_DIR) && rm -rf bin/boot
	rm -f $(EXT2_IMG)
