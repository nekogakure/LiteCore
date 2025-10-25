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

CFLAGS     = -ffreestanding -m32 -c -Wall -Wextra -I$(INCLUDE)
LDFLAGS    = -m elf_i386
NFLAGS     = -f bin
QEMU_FLAGS = -monitor stdio
CONSOLE    = -display curses

SOURCES    = $(shell find $(SRC_KERNEL) -name "*.c")
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
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


$(BOOT): $(SRC_BOOT)/boot.asm
	$(NASM) $(NFLAGS) $< -o $@

run: $(IMG)
	make clean
	make all
	$(QEMU) $(QEMU_FLAGS) -fda $<

run-console: $(IMG)
	make clean
	make all
	$(QEMU) $(CONSOLE) -fda $< 

clean:
	rm -rf $(OUT_DIR)

.PHONY: all run clean
