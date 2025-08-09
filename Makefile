LOADER_DIR = loader
KERNEL_DIR = kernel
BUILD_DIR = boot


LOADER_ASM = $(LOADER_DIR)/boot.asm
KERNEL_START_ASM = $(KERNEL_DIR)/start.asm
KERNEL_C_SRCS = $(KERNEL_DIR)/k_main.c $(KERNEL_DIR)/kernel.c $(KERNEL_DIR)/util/vga.c
KERNEL_OBJS = $(BUILD_DIR)/start.o $(BUILD_DIR)/k_main.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/vga.o

IMG = $(BUILD_DIR)/litecore.img
BOOT_BIN = $(BUILD_DIR)/boot.bin
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
KERNEL_ELF = $(BUILD_DIR)/kernel.elf

CFLAGS = -m64 -ffreestanding -fno-pic -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -I$(KERNEL_DIR) -I$(KERNEL_DIR)/util

all: $(IMG)

$(BOOT_BIN): $(LOADER_ASM)
	mkdir -p $(BUILD_DIR)
	nasm -f bin -o $@ $<

$(BUILD_DIR)/start.o: $(KERNEL_START_ASM)
	mkdir -p $(BUILD_DIR)
	nasm -f elf64 -o $@ $<

$(BUILD_DIR)/k_main.o: $(KERNEL_DIR)/k_main.c $(KERNEL_DIR)/kernel.h $(KERNEL_DIR)/util/vga.h $(KERNEL_DIR)/util/config.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.o: $(KERNEL_DIR)/kernel.c $(KERNEL_DIR)/kernel.h $(KERNEL_DIR)/util/vga.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/vga.o: $(KERNEL_DIR)/util/vga.c $(KERNEL_DIR)/util/vga.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): $(KERNEL_OBJS)
	ld -m elf_x86_64 -T $(KERNEL_DIR)/linker.ld -o $(KERNEL_ELF) $^
	objcopy -O binary $(KERNEL_ELF) $@
	@echo "\033[0;32mKernel size: $$(wc -c < $@) bytes\033[0m"
	@echo "entry: 0x1000(16bit mode)"

$(IMG): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $(IMG)
	truncate -s 10M $(IMG)

run: $(IMG)
	qemu-system-x86_64 -hda $(IMG) -monitor stdio -cpu qemu64 -no-reboot -no-shutdown

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean run
