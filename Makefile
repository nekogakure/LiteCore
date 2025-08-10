LOADER_DIR = loader
KERNEL_DIR = kernel
MEM_DIR = mem
ARCH_DIR = $(KERNEL_DIR)/arch
BUILD_DIR = boot

LOADER_ASM = loader/boot.asm
KERNEL_START_ASM = kernel/start.asm
KERNEL_C_SRCS = mem/memory.c mem/paging.c kernel/panic.c kernel/kernel.c kernel/k_main.c kernel/util/vga.c kernel/arch/arch.c
OTHER_ASM_SRCS = kernel/arch/mode_switch.asm
KERNEL_OBJS = boot/start.o boot/memory.o boot/paging.o boot/panic.o boot/kernel.o boot/k_main.o boot/vga.o boot/arch.o boot/mode_switch.o

IMG = $(BUILD_DIR)/litecore.img
BOOT_BIN = $(BUILD_DIR)/boot.bin
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
KERNEL_ELF = $(BUILD_DIR)/kernel.elf

CFLAGS = -m64 -ffreestanding -fno-pic -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Ikernel -Ikernel/util -Imem -g -O0

all: $(IMG)

$(BOOT_BIN): $(LOADER_ASM)
	mkdir -p $(BUILD_DIR)
	nasm -f bin -o $@ $<

$(BUILD_DIR)/start.o: $(KERNEL_START_ASM)
	mkdir -p $(BUILD_DIR)
	nasm -f elf64 -g -F dwarf -o $@ $< -w-gnu-stack

$(BUILD_DIR)/mode_switch.o: kernel/arch/mode_switch.asm
	mkdir -p $(BUILD_DIR)
	nasm -f elf64 -g -F dwarf -o $@ $< -w-gnu-stack

$(BUILD_DIR)/memory.o: mem/memory.c mem/memory.h mem/config.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/paging.o: mem/paging.c mem/paging.h mem/memory.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/panic.o: kernel/panic.c kernel/panic.h kernel/util/vga.h kernel/util/config.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.o: kernel/kernel.c kernel/kernel.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/k_main.o: kernel/k_main.c kernel/util/vga.h kernel/util/config.h kernel/kernel.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/vga.o: kernel/util/vga.c kernel/util/vga.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/arch.o: kernel/arch/arch.c kernel/arch/arch.h
	mkdir -p $(BUILD_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): $(KERNEL_OBJS)
	ld -m elf_x86_64 -T $(KERNEL_DIR)/linker.ld -o $(KERNEL_ELF) --build-id=none -g $^ -z noexecstack
	objcopy -O binary $(KERNEL_ELF) $@
	@echo "\033[0;32mKernel size: $$(wc -c < $@) bytes\033[0m"
	@echo "entry: 0x1000(16bit mode)"

$(IMG): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $(IMG)
	truncate -s 10M $(IMG)

run: $(IMG)
	qemu-system-x86_64 -drive file=$(IMG),format=raw -monitor stdio -cpu qemu64 -no-reboot -no-shutdown

run-debug: $(IMG)
	qemu-system-x86_64 -drive file=$(IMG),format=raw -monitor stdio -cpu qemu64 -no-reboot -no-shutdown -d int,cpu_reset,exec,in_asm,page -D qemu.log

debug: $(IMG) clean-qemu
	@echo "Starting QEMU with GDB server on port 1234..."
	qemu-system-x86_64 -drive file=$(IMG),format=raw -s -S -cpu qemu64 -no-reboot -no-shutdown &
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

.PHONY: all clean run run-debug debug memory-map
