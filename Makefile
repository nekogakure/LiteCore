CC         = gcc
LD         = ld
NASM       = nasm
QEMU       = qemu-system-x86_64
OBJCOPY    = objcopy

SRC_DIR    = src
SRC_BOOT   = $(SRC_DIR)/boot
SRC_KERNEL = $(SRC_DIR)/kernel
OUT_DIR    = bin

CFLAGS     = -ffreestanding -m32 -c -Wall -Wextra
LDFLAGS    = -m elf_i386
NFLAGS     = -f bin
QEMU_FLAGS = -monitor stdio

SOURCES    = $(wildcard $(SRC_KERNEL)/*.c)
OBJECTS    = $(patsubst $(SRC_KERNEL)/%.c, $(OUT_DIR)/%.o, $(SOURCES))

BOOT       = $(OUT_DIR)/boot.bin
KERNEL_ELF = $(OUT_DIR)/kernel.elf
KERNEL     = $(OUT_DIR)/kernel.bin
IMG        = $(OUT_DIR)/disk.img
LINKER     = $(SRC_DIR)/kernel.ld

all: $(OUT_DIR) $(IMG)

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

$(IMG): $(BOOT) $(KERNEL)
	cat $^ > $@

$(KERNEL_ELF): $(OBJECTS) $(LINKER)
	$(LD) $(LDFLAGS) -T $(LINKER) $(OBJECTS) -o $@

$(KERNEL): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

$(OUT_DIR)/%.o: $(SRC_KERNEL)/%.c
	$(CC) $(CFLAGS) $< -o $@

$(BOOT): $(SRC_BOOT)/boot.asm
	$(NASM) $(NFLAGS) $< -o $@

run: $(IMG)
	make clean
	make all
	$(QEMU) $(QEMU_FLAGS) -fda $<

clean:
	rm -rf $(OUT_DIR)

.PHONY: all run clean
