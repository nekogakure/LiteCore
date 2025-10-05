# LiteCore Kernel Makefile

# フラグ
QEMU = qemu-system-x86_64
QEMU_FLAGS = -m 256M -serial stdio -machine q35 -cpu qemu64 \
             -drive if=pflash,format=raw,readonly=on,file=/usr/share/ovmf/OVMF_CODE.fd \
             -drive if=pflash,format=raw,file=/tmp/ovmf_vars.fd \
             -drive format=raw,file=$(UEFI_IMAGE) \
             -netdev user,id=net0 -device e1000,netdev=net0 \
             -monitor unix:/tmp/qemu-monitor,server,nowaitC = gcc
LD = ld
OBJCOPY = objcopy

# コンパイルフラグ
CFLAGS = -std=c99 -ffreestanding -fno-stack-protector -fno-pic -mno-sse \
         -mno-sse2 -mno-mmx -mno-80387 -mno-red-zone -mcmodel=large \
         -Wall -Wextra -g -I./include -DDEBUG

# リンカーフラグ
LDFLAGS = -T kernel.ld -nostdlib -z max-page-size=0x1000

# ディレクトリ設定
BUILD_DIR = build
BOOT_DIR = boot
KERNEL_DIR = kernel
UTIL_DIR = util

# ソースファイル
BOOT_SOURCES = $(wildcard $(BOOT_DIR)/*.c)
BOOT_ASM_SOURCES = $(wildcard $(BOOT_DIR)/*.S)
KERNEL_SOURCES = $(wildcard $(KERNEL_DIR)/*.c)
UTIL_SOURCES = $(wildcard $(UTIL_DIR)/*.c)

# オブジェクトファイル
BOOT_OBJECTS = $(BOOT_SOURCES:$(BOOT_DIR)/%.c=$(BUILD_DIR)/boot_%.o)
BOOT_ASM_OBJECTS = $(BOOT_ASM_SOURCES:$(BOOT_DIR)/%.S=$(BUILD_DIR)/boot_%.o)
KERNEL_OBJECTS = $(KERNEL_SOURCES:$(KERNEL_DIR)/%.c=$(BUILD_DIR)/kernel_%.o)
UTIL_OBJECTS = $(UTIL_SOURCES:$(UTIL_DIR)/%.c=$(BUILD_DIR)/util_%.o)

ALL_OBJECTS = $(BOOT_OBJECTS) $(BOOT_ASM_OBJECTS) $(KERNEL_OBJECTS) $(UTIL_OBJECTS)

# ターゲット
TARGET = $(BUILD_DIR)/litecore.elf
KERNEL_BIN = $(BUILD_DIR)/litecore.bin
ISO_IMAGE = $(BUILD_DIR)/litecore.iso

# QEMU設定
QEMU = qemu-system-x86_64
QEMU_FLAGS = -m 256M -serial stdio -machine pc -cpu qemu64 \
             -drive format=raw,file=$(ISO_IMAGE)

.PHONY: all clean run check-tools

all: $(TARGET)

# 必要なツールの確認
check-tools:
	@echo "Checking required tools..."
	@which $(CC) > /dev/null || (echo "Error: $(CC) not found" && exit 1)
	@which $(LD) > /dev/null || (echo "Error: $(LD) not found" && exit 1)
	@which $(OBJCOPY) > /dev/null || (echo "Error: $(OBJCOPY) not found" && exit 1)
	@which $(QEMU) > /dev/null || (echo "Error: $(QEMU) not found. Install qemu-system-x86_64" && exit 1)
	@which grub-mkrescue > /dev/null || (echo "Error: grub-mkrescue not found. Install grub2-tools" && exit 1)
	@echo "All required tools are available"

$(TARGET): $(ALL_OBJECTS) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJECTS)

# イメージを作成
$(ISO_IMAGE): $(TARGET) | $(BUILD_DIR)
	@echo "Creating bootable ISO image..."
	mkdir -p $(BUILD_DIR)/isodir/boot/grub
	cp $(TARGET) $(BUILD_DIR)/isodir/boot/litecore.elf
	echo 'menuentry "LiteCore" {' > $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	echo '    multiboot2 /boot/litecore.elf' >> $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	echo '    boot' >> $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	echo '}' >> $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD_DIR)/isodir

$(BUILD_DIR)/boot_%.o: $(BOOT_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/boot_%.o: $(BOOT_DIR)/%.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel_%.o: $(KERNEL_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/util_%.o: $(UTIL_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

run: check-tools $(ISO_IMAGE)
	@echo "Starting LiteCore kernel in QEMU..."
	@echo "Debug output will appear in this terminal"
	@echo "Press Ctrl+C to quit QEMU"
	$(QEMU) $(QEMU_FLAGS)

info:
	@echo "LiteCore Kernel Build Information"
	@echo "Target: $(TARGET)"
	@echo "Boot sources: $(BOOT_SOURCES)"
	@echo "Kernel sources: $(KERNEL_SOURCES)"
	@echo "Utility sources: $(UTIL_SOURCES)"

help:
	@echo "LiteCore Kernel Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all         - Build the kernel (default)"
	@echo "  clean       - Remove build artifacts"
	@echo "  run         - Build and run kernel in QEMU"
	@echo "  check-tools - Check if required tools are installed"
	@echo "  info        - Show build information"
	@echo "  help        - Show this help message"
	@echo ""
	@echo "Requirements:"
	@echo "  - gcc, ld, objcopy (binutils)"
	@echo "  - qemu-system-x86_64"
	@echo "  - grub2-tools (grub-mkrescue)"