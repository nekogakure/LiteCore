NASM       = nasm
QEMU       = qemu-system-x86_64

NFLAGS     = -f bin
QEMU_FLAGS = -monitor stdio

SRC_BOOT   = src/boot
SRC_KERNEL = src/kernel
OUT_DIR    = bin

BOOT       = $(OUT_DIR)/boot.bin
KERNEL     = $(OUT_DIR)/kernel.bin
IMG        = $(OUT_DIR)/disk.img

all: $(OUT_DIR) $(IMG)

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

$(IMG): $(BOOT) $(KERNEL)
	cat $(BOOT) $(KERNEL) > $@

$(BOOT): $(SRC_BOOT)/boot.asm
	$(NASM) $(NFLAGS) $< -o $@

$(KERNEL): $(SRC_KERNEL)/main.asm
	$(NASM) $(NFLAGS) $< -o $@

run: $(IMG)
	make all
	$(QEMU) $(QEMU_FLAGS) -fda $<

clean:
	rm -rf $(OUT_DIR)

.PHONY: all run clean
