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

CFLAGS     = -O2 -Wimplicit-function-declaration -ffreestanding -m32 -c -Wall -Wextra -I$(INCLUDE)
LDFLAGS    = -m elf_i386
NFLAGS     = -f bin
QEMU_FLAGS = -monitor stdio -no-reboot -d int,guest_errors -D kernel.log
QEMU_SERIAL = -serial file:kernel_console.log -display none
CONSOLE    = -display curses

SOURCES    = $(shell find $(SRC_KERNEL) -name "*.c")
ASM_SOURCES = $(shell find $(SRC_KERNEL) -name "*.asm")
OBJECTS    = $(shell printf "%s\n" $(patsubst $(SRC_KERNEL)/%.c, $(OUT_DIR)/%.o, $(SOURCES)) $(patsubst $(SRC_KERNEL)/%.asm, $(OUT_DIR)/%.o, $(ASM_SOURCES)) | sort -u)

BOOT       = $(OUT_DIR)/boot.bin
KERNEL_ELF = $(OUT_DIR)/kernel.elf
KERNEL     = $(OUT_DIR)/kernel.bin
IMG        = $(OUT_DIR)/disk.img
LINKER     = $(SRC_DIR)/kernel.ld

CALC_SCRIPT = $(SRC_BOOT)/config.inc

.PHONY: calculate-sectors
calculate-sectors: $(KERNEL)
	@echo "Generating $(SRC_BOOT)/config.inc (based on $(KERNEL) size)"
	@size=$$(wc -c < $(KERNEL)); sectors=$$(( (size + 511) / 512 )); \
	printf "KERNEL_OFFSET EQU 0x10000       ; カーネルをロードするアドレス\nSECTOR_COUNT  EQU %s            ; 読み込むセクタの数\nSTART_SECTOR  EQU 2             ; 開始するセクタ番号\nCYLINDER_NUM  EQU 0             ; シリンダ番号\nHEAD_NUM      EQU 0             ; ヘッド番号\nCODE_SEGMENT  EQU 0x08          ; コードセグメント\nDATA_SEGMENT  EQU 0x10          ; データセメント\n" "$$sectors" > $(SRC_BOOT)/config.inc


all: $(OUT_DIR) $(IMG)

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

$(KERNEL_ELF): $(OBJECTS) $(LINKER)
	$(LD) $(LDFLAGS) -T $(LINKER) $(OBJECTS) -o $@

$(KERNEL): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

$(BOOT): $(SRC_BOOT)/boot.asm calculate-sectors
	$(NASM) $(NFLAGS) $< -o $@

$(IMG): $(BOOT) $(KERNEL)
	cat $^ > $@

$(OUT_DIR)/%.o: $(SRC_KERNEL)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


$(OUT_DIR)/%.o: $(SRC_KERNEL)/%.asm
	mkdir -p $(dir $@)
	$(NASM) -f elf32 $< -o $@

src/ext2.img:
	@echo "Creating ext2.img (2MB)..."
	@python3 tools/mk_ext2_image.py src/ext2.img 2048 example/

run: $(IMG) src/ext2.img
	make all
	$(QEMU) $(QEMU_FLAGS) -drive file=$(IMG),format=raw,if=floppy -drive file=src/ext2.img,format=raw,if=ide

run-console: $(IMG) src/ext2.img
	make all
	$(QEMU) $(CONSOLE) -drive file=$(IMG),format=raw,if=floppy -drive file=src/ext2.img,format=raw,if=ide
run-serial: $(IMG) src/ext2.img
	make all
	$(QEMU) $(QEMU_SERIAL) -drive file=$(IMG),format=raw,if=floppy -drive file=src/ext2.img,format=raw,if=ide

clean:
	rm -rf $(OUT_DIR)

.PHONY: all run clean
