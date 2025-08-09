SRC_DIR = .
BUILD_DIR = boot

ASM_SRC = $(SRC_DIR)/boot.asm
START_ASM = $(SRC_DIR)/start.asm
C_SRCS = $(SRC_DIR)/k_main.c $(SRC_DIR)/vga.c $(SRC_DIR)/kernel.c
OBJS = $(BUILD_DIR)/start.o $(BUILD_DIR)/k_main.o $(BUILD_DIR)/vga.o $(BUILD_DIR)/kernel.o

IMG = $(BUILD_DIR)/litecore.img
KERNEL = $(BUILD_DIR)/kernel.bin

all: $(IMG)

$(BUILD_DIR)/boot.bin: $(ASM_SRC)
	mkdir -p $(BUILD_DIR)
	nasm -f bin -o $@ $<

$(BUILD_DIR)/start.o: $(START_ASM)
	mkdir -p $(BUILD_DIR)
	nasm -f elf64 -o $@ $<

$(BUILD_DIR)/k_main.o: $(SRC_DIR)/k_main.c $(SRC_DIR)/vga.h $(SRC_DIR)/kernel.h
	mkdir -p $(BUILD_DIR)
	gcc -m64 -ffreestanding -fno-pic -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -c $< -o $@

$(BUILD_DIR)/vga.o: $(SRC_DIR)/vga.c $(SRC_DIR)/vga.h
	mkdir -p $(BUILD_DIR)
	gcc -m64 -ffreestanding -fno-pic -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -c $< -o $@

$(BUILD_DIR)/kernel.o: $(SRC_DIR)/kernel.c $(SRC_DIR)/kernel.h $(SRC_DIR)/vga.h
	mkdir -p $(BUILD_DIR)
	gcc -m64 -ffreestanding -fno-pic -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -c $< -o $@

$(KERNEL): $(OBJS)
	ld -m elf_x86_64 -T linker.ld -o $(BUILD_DIR)/kernel.elf $^
	objcopy -O binary $(BUILD_DIR)/kernel.elf $@
	@echo "\033[0;32mKernel size: $$(wc -c < $@) bytes\033[0m"
	@echo "entry: 0x1000(16bit mode)"

$(IMG): $(BUILD_DIR)/boot.bin $(KERNEL)
	cat $(BUILD_DIR)/boot.bin $(KERNEL) > $(IMG)
	truncate -s 10M $(IMG)

run: $(IMG)
	qemu-system-x86_64 -hda $(IMG) -monitor stdio -cpu qemu64 -no-reboot -no-shutdown

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean run
