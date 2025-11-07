CC         = gcc
LD         = ld
NASM       = nasm
QEMU       = qemu-system-x86_64
OBJCOPY    = objcopy

SRC_DIR    = src
SRC_BOOT   = $(SRC_DIR)/boot
SRC_KERNEL = $(SRC_DIR)/kernel
INCLUDE    = $(SRC_DIR)/include
OUT_DIR    = bin
K_OUT_DIR    = $(OUT_DIR)/kernel
B_OUT_DIR    = $(OUT_DIR)/boot
IMG_OUT_DIR = $(OUT_DIR)

CFLAGS     = -O2 -Wimplicit-function-declaration -ffreestanding -m32 -c -Wall -Wextra -I$(INCLUDE)
LDFLAGS    = -m elf_i386
NFLAGS     = -f bin
QEMU_FLAGS = -monitor stdio -no-reboot -d int,guest_errors -D kernel.log
QEMU_SERIAL = -serial file:kernel_console.log -display none
CONSOLE    = -display curses

SOURCES    = $(shell find $(SRC_KERNEL) -name "*.c")
ASM_SOURCES = $(shell find $(SRC_KERNEL) -name "*.asm")
OBJECTS    = $(shell printf "%s\n" $(patsubst $(SRC_KERNEL)/%.c, $(K_OUT_DIR)/%.o, $(SOURCES)) $(patsubst $(SRC_KERNEL)/%.asm, $(K_OUT_DIR)/%.o, $(ASM_SOURCES)) | sort -u)

BOOT       = $(B_OUT_DIR)/boot.bin
KERNEL_ELF = $(K_OUT_DIR)/kernel.elf
KERNEL     = $(K_OUT_DIR)/kernel.bin
IMG        = $(B_OUT_DIR)/boot.img
LITECORE_IMG = $(IMG_OUT_DIR)/LiteCore.img
LINKER     = $(SRC_DIR)/kernel.ld

CALC_SCRIPT = $(SRC_BOOT)/config.inc

.PHONY: all run run-console run-serial clean calculate-sectors
.DEFAULT_GOAL := all

calculate-sectors: $(KERNEL)
	@echo "Generating $(SRC_BOOT)/config.inc (based on $(KERNEL) size)"
	@size=$$(wc -c < $(KERNEL)); sectors=$$(( (size + 511) / 512 )); \
	printf "KERNEL_OFFSET EQU 0x10000       ; カーネルをロードするアドレス\nSECTOR_COUNT  EQU %s            ; 読み込むセクタの数\nSTART_SECTOR  EQU 2             ; 開始するセクタ番号\nCYLINDER_NUM  EQU 0             ; シリンダ番号\nHEAD_NUM      EQU 0             ; ヘッド番号\nCODE_SEGMENT  EQU 0x08          ; コードセグメント\nDATA_SEGMENT  EQU 0x10          ; データセメント\n" "$$sectors" > $(SRC_BOOT)/config.inc


all: $(K_OUT_DIR) $(B_OUT_DIR) $(IMG) $(LITECORE_IMG)

$(K_OUT_DIR):
	mkdir -p $(K_OUT_DIR)

$(B_OUT_DIR):
	mkdir -p $(B_OUT_DIR)

$(KERNEL_ELF): $(OBJECTS) $(LINKER)
	$(LD) $(LDFLAGS) -T $(LINKER) $(OBJECTS) -o $@

$(KERNEL): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

$(BOOT): $(SRC_BOOT)/boot.asm calculate-sectors
	$(NASM) $(NFLAGS) $< -o $@

$(IMG): $(BOOT) $(KERNEL)
	cat $^ > $@

$(K_OUT_DIR)/%.o: $(SRC_KERNEL)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


$(K_OUT_DIR)/%.o: $(SRC_KERNEL)/%.asm
	mkdir -p $(dir $@)
	$(NASM) -f elf32 $< -o $@

$(LITECORE_IMG):
	@echo "Creating LiteCore.img (2MB ext2 filesystem)..."
	@python3 tools/mk_ext2_image.py $(LITECORE_IMG) 2048 bin

run: $(K_OUT_DIR) $(B_OUT_DIR) $(IMG) $(LITECORE_IMG)
	make all
	$(QEMU) $(QEMU_FLAGS) -drive file=$(IMG),format=raw,if=floppy -drive file=$(LITECORE_IMG),format=raw,if=ide

run-console: $(K_OUT_DIR) $(B_OUT_DIR) $(IMG) $(LITECORE_IMG)
	make all
	$(QEMU) $(CONSOLE) -drive file=$(IMG),format=raw,if=floppy -drive file=$(LITECORE_IMG),format=raw,if=ide

run-serial: $(K_OUT_DIR) $(B_OUT_DIR) $(IMG) $(LITECORE_IMG)
	make all
	$(QEMU) $(QEMU_SERIAL) -drive file=$(IMG),format=raw,if=floppy -drive file=$(LITECORE_IMG),format=raw,if=ide

clean:
	rm -rf $(OUT_DIR)
