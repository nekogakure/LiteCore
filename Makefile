CC         = gcc
LD         = ld
NASM       = nasm
QEMU       = qemu-system-x86_64
OBJCOPY    = objcopy

# GNU-EFI paths (adjust if needed)
EFI_INC    = /usr/include/efi
EFI_LIB    = /usr/lib
EFI_CRT    = $(EFI_LIB)/crt0-efi-x86_64.o
EFI_LDS    = $(EFI_LIB)/elf_x86_64_efi.lds

SRC_DIR    = src
SRC_BOOT   = $(SRC_DIR)/boot
SRC_KERNEL = $(SRC_DIR)/kernel
INCLUDE    = $(SRC_DIR)/include
OUT_DIR    = bin
K_OUT_DIR    = $(OUT_DIR)/kernel
B_OUT_DIR    = $(OUT_DIR)/boot
IMG_OUT_DIR = $(OUT_DIR)

# Kernel flags (64-bit)
CFLAGS     = -O2 -Wimplicit-function-declaration -Wunused-but-set-variable -ffreestanding -m64 -c -Wall -Wextra -I$(INCLUDE) -mcmodel=large -mno-red-zone -fno-pic
LDFLAGS    = -m elf_x86_64 -z max-page-size=0x1000

# UEFI bootloader flags (64-bit)
EFI_CFLAGS = -fpic -ffreestanding -fno-stack-protector -fno-stack-check \
             -fshort-wchar -mno-red-zone -I$(EFI_INC) -I$(EFI_INC)/x86_64 \
             -DGNU_EFI_USE_MS_ABI
EFI_LDFLAGS = -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L$(EFI_LIB)

NFLAGS     = -f bin
QEMU_FLAGS = -serial stdio -display none -monitor none -device qemu-xhci,id=xhci \
             -bios /usr/share/ovmf/OVMF.fd
QEMU_VGA   = -display curses -device qemu-xhci,id=xhci -bios /usr/share/ovmf/OVMF.fd
CONSOLE    = -display curses

SOURCES    = $(shell find $(SRC_KERNEL) -name "*.c")
ASM_SOURCES = $(shell find $(SRC_KERNEL) -name "*.asm")
OBJECTS    = $(shell printf "%s\n" $(patsubst $(SRC_KERNEL)/%.c, $(K_OUT_DIR)/%.o, $(SOURCES)) $(patsubst $(SRC_KERNEL)/%.asm, $(K_OUT_DIR)/%.o, $(ASM_SOURCES)) | sort -u)

BOOTX64    = $(B_OUT_DIR)/BOOTX64.EFI
KERNEL_ELF = $(K_OUT_DIR)/kernel.elf
KERNEL     = $(K_OUT_DIR)/kernel.bin
ESP_IMG    = $(IMG_OUT_DIR)/LiteCore.img
LINKER     = $(SRC_DIR)/kernel.ld

.PHONY: all run run-console run-vga clean
.DEFAULT_GOAL := all

all: $(K_OUT_DIR) $(B_OUT_DIR) $(B_OUT_DIR) $(BOOTX64) $(KERNEL) $(ESP_IMG)

$(K_OUT_DIR):
	mkdir -p $(K_OUT_DIR)

$(B_OUT_DIR):
	mkdir -p $(B_OUT_DIR)
	mkdir -p $(B_OUT_DIR)/EFI/BOOT

# Kernel build (32-bit)
$(KERNEL_ELF): $(OBJECTS) $(LINKER)
	$(LD) $(LDFLAGS) -T $(LINKER) $(OBJECTS) -o $@

$(KERNEL): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@
	@echo "Kernel size: $$(wc -c < $@) bytes"

# UEFI Bootloader build (64-bit)
$(B_OUT_DIR)/efi_boot.o: $(SRC_BOOT)/efi_boot.c
	$(CC) $(EFI_CFLAGS) -c $< -o $@

$(B_OUT_DIR)/efi_boot.so: $(B_OUT_DIR)/efi_boot.o
	$(LD) $(EFI_LDFLAGS) $(EFI_CRT) $< -o $@ -lefi -lgnuefi

$(BOOTX64): $(B_OUT_DIR)/efi_boot.so
	$(OBJCOPY) -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym -j .rel \
	           -j .rela -j .reloc --target=efi-app-x86_64 $< $@
	@echo "UEFI bootloader created: $@"

# Kernel object files
$(K_OUT_DIR)/%.o: $(SRC_KERNEL)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(K_OUT_DIR)/%.o: $(SRC_KERNEL)/%.asm
	mkdir -p $(dir $@)
	$(NASM) -f elf64 $< -o $@

# Create ESP (EFI System Partition) image
$(ESP_IMG): $(BOOTX64) $(KERNEL)
	@echo "Creating UEFI ESP image..."
	@rm -f $(ESP_IMG)
	@dd if=/dev/zero of=$(ESP_IMG) bs=1M count=64 2>/dev/null
	@mkfs.vfat -F 32 $(ESP_IMG) >/dev/null 2>&1
	@mkdir -p /tmp/litecore_mnt
	@sudo mount -o loop $(ESP_IMG) /tmp/litecore_mnt
	@sudo mkdir -p /tmp/litecore_mnt/EFI/BOOT
	@sudo cp $(BOOTX64) /tmp/litecore_mnt/EFI/BOOT/
	@sudo cp $(KERNEL) /tmp/litecore_mnt/
	@sudo umount /tmp/litecore_mnt
	@rmdir /tmp/litecore_mnt
	@echo "ESP image created: $(ESP_IMG)"

run: $(ESP_IMG)
	$(QEMU) $(QEMU_FLAGS) -drive file=$(ESP_IMG),format=raw,if=ide -nographic

run-console: $(ESP_IMG)
	$(QEMU) $(CONSOLE) -device qemu-xhci,id=xhci -drive file=$(ESP_IMG),format=raw,if=ide

run-vga: $(ESP_IMG)
	$(QEMU) $(QEMU_VGA) -drive file=$(ESP_IMG),format=raw,if=ide

clean:
	rm -rf $(OUT_DIR)
