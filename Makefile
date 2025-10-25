NASM       = nasm
QEMU       = qemu-system-x86_64

NFLAGS     = -f bin
QEMU_FLAGS = -monitor stdio

all:
	make clean
	mkdir -p build
	$(NASM) $(NFLAGS) src/boot/boot.asm -o build/boot.bin

run:
	make all
	$(QEMU) $(QEMU_FLAGS) -fda build/boot.bin

clean:
	rm -rf build/

.PHONY: all run clean