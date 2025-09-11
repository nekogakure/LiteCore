import os
import glob
import sys
from typing import List, Dict, Set, Tuple, Optional

class MakefileGenerator:
    def __init__(self, project_root: str):
        self.project_root = project_root
        self.build_dir = "bin"
        self.loader_dir = "boot"
        self.kernel_dir = "kernel"
        self.cflags = "-m64 -ffreestanding -fno-pic -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2"
        self.debug_flags = "-g -O0"
        self.img = f"{self.build_dir}/litecore.img"
        self.boot_bin = f"{self.build_dir}/boot.bin"
        self.kernel_bin = f"{self.build_dir}/kernel.bin"
        self.kernel_elf = f"{self.build_dir}/kernel.elf"
        self.asm_files = []
        self.c_files = []
        self.h_files = []
        self.include_dirs = set()

    def find_files(self):
        for root, _, files in os.walk(self.project_root):
            if ".git" in root or self.build_dir in root:
                continue
            
            for file in files:
                full_path = os.path.join(root, file)
                rel_path = os.path.relpath(full_path, self.project_root)
                
                if file.endswith(".c"):
                    self.c_files.append(rel_path)
                    # Add directory to include paths
                    dir_path = os.path.dirname(rel_path)
                    if dir_path:
                        self.include_dirs.add(dir_path)
                elif file.endswith(".h"):
                    self.h_files.append(rel_path)
                    # Add directory to include paths
                    dir_path = os.path.dirname(rel_path)
                    if dir_path:
                        self.include_dirs.add(dir_path)
                elif file.endswith(".asm"):
                    self.asm_files.append(rel_path)

    def get_include_flags(self) -> str:
        if not self.include_dirs:
            return ""
        include_flags = " ".join([f"-I{dir}" for dir in sorted(self.include_dirs)])
        return include_flags

    def get_dependencies(self, c_file: str) -> List[str]:
        deps = []
        try:
            with open(os.path.join(self.project_root, c_file), 'r') as f:
                for line in f:
                    line = line.strip()
                    if line.startswith('#include "'):
                        include = line.split('"')[1]
                        file_dir = os.path.dirname(c_file)
                        if "/" in include:
                            include_path = os.path.normpath(os.path.join(file_dir, include))
                            for h_file in self.h_files:
                                if h_file.endswith(include):
                                    deps.append(h_file)
                                    break
                        else:
                            include_path = os.path.join(file_dir, include)
                            if include_path in self.h_files:
                                deps.append(include_path)
                            else:
                                for h_file in self.h_files:
                                    if h_file.endswith(include):
                                        deps.append(h_file)
                                        break
        except Exception as e:
            print(f"Warn: {c_file} : {e}", file=sys.stderr)
        return deps


    def generate_makefile(self) -> str:
        # GRUB対応：start.asmは不要（C言語で_startを実装）
        other_asm_files = [f for f in self.asm_files 
                          if not f.startswith(self.loader_dir)]
        kernel_c_srcs = [f for f in self.c_files if not f.startswith(self.loader_dir)]
        kernel_c_srcs_var = " ".join(kernel_c_srcs)
        
        # Build kernel objects list（C言語のみ）
        kernel_objs = []
        for src in kernel_c_srcs:
            obj_name = os.path.basename(src).replace(".c", ".o")
            kernel_objs.append(f"{self.build_dir}/{obj_name}")
        
        # その他のASMファイルがあれば追加
        for asm_file in other_asm_files:
            obj_name = os.path.basename(asm_file).replace(".asm", ".o")
            obj_path = f"{self.build_dir}/{obj_name}"
            if obj_path not in kernel_objs:
                kernel_objs.append(obj_path)
        
        kernel_objs_var = " ".join(kernel_objs)
        other_asm_files_var = " ".join(other_asm_files)
        include_flags = self.get_include_flags()
        
        makefile_content = f"""# GRUB対応 LiteCore Makefile (C言語ベース)
KERNEL_DIR = {self.kernel_dir}
BUILD_DIR = {self.build_dir}

KERNEL_C_SRCS = {kernel_c_srcs_var}
OTHER_ASM_SRCS = {other_asm_files_var}
KERNEL_OBJS = {kernel_objs_var}

KERNEL_BIN = $(BUILD_DIR)/kernel.bin
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
ISO = $(BUILD_DIR)/litecore.iso

VERSION = $(shell cat version.txt 2>/dev/null || echo "dev")
CFLAGS = {self.cflags} {include_flags} {self.debug_flags} -DKERNEL_VERSION=\\"$(VERSION)\\"

all: $(ISO)

kernel: $(KERNEL_BIN)

"""
        
        # その他のASMファイルがあれば処理
        for asm_file in other_asm_files:
            obj_name = os.path.basename(asm_file).replace(".asm", ".o")
            obj_path = f"$(BUILD_DIR)/{obj_name}"
            makefile_content += f"{obj_path}: {asm_file}\n"
            makefile_content += "\tmkdir -p $(BUILD_DIR)\n"
            makefile_content += "\tnasm -f elf64 -g -F dwarf -o $@ $< -w-gnu-stack\n\n"
            
        # C言語ファイルのコンパイル
        for c_file in kernel_c_srcs:
            obj_name = os.path.basename(c_file).replace(".c", ".o")
            obj_path = f"$(BUILD_DIR)/{obj_name}"
            deps = self.get_dependencies(c_file)
            deps_str = " ".join(deps) if deps else ""
            makefile_content += f"{obj_path}: {c_file} {deps_str}\n"
            makefile_content += "\tmkdir -p $(BUILD_DIR)\n"
            makefile_content += "\tgcc $(CFLAGS) -c $< -o $@\n\n"
        makefile_content += """
$(KERNEL_BIN): $(KERNEL_OBJS)
\tld -m elf_x86_64 -T $(KERNEL_DIR)/linker.ld -o $(KERNEL_ELF) --build-id=none -g $^ -z noexecstack
\tobjcopy -O binary $(KERNEL_ELF) $@
\t@echo "\\033[0;32mKernel size: $$(wc -c < $@) bytes\\033[0m"

$(ISO): $(KERNEL_BIN)
\tmkdir -p $(BUILD_DIR)/iso/boot/grub
\tcp $(KERNEL_BIN) $(BUILD_DIR)/iso/boot/
\tcp grub/grub.cfg $(BUILD_DIR)/iso/boot/grub/
\tgrub-mkrescue -o $(ISO) $(BUILD_DIR)/iso
\t@echo "\\033[0;32mGRUB ISO created: $(ISO)\\033[0m"

run: $(ISO)
\tqemu-system-x86_64 -cdrom $(ISO) -monitor stdio -cpu qemu64 -no-reboot -no-shutdown

run-debug: $(ISO)
\tqemu-system-x86_64 -cdrom $(ISO) -monitor stdio -cpu qemu64 -no-reboot -no-shutdown -d int,cpu_reset,exec,in_asm,page -D qemu.log

debug: $(ISO) clean-qemu
\t@echo "Starting QEMU with GDB server on port 1234..."
\tqemu-system-x86_64 -cdrom $(ISO) -s -S -cpu qemu64 -no-reboot -no-shutdown &
\t@sleep 1
\t@echo "Starting GDB and connecting to QEMU..."
\tgdb -ex "file $(KERNEL_ELF)" -ex "target remote localhost:1234" -ex "set disassembly-flavor intel" -ex "layout asm" -ex "break *0x1000" -ex "continue"

clean-qemu:
\t@echo "Killing any existing QEMU processes..."
\t@-pkill -9 qemu-system-x86_64 2>/dev/null || true
\t@sleep 1

clean:
\trm -rf $(BUILD_DIR)

memory-map: $(KERNEL_ELF)
\t@echo "======== Kernel memory map ========"
\t@objdump -h $(KERNEL_ELF)
\t@echo "\\n======== Kernel symbol table ========"
\t@nm $(KERNEL_ELF) | sort
\t@echo "\\n======== Entry point & sections ========"
\t-@readelf -h $(KERNEL_ELF) | grep "Entry point"
\t@readelf -S $(KERNEL_ELF)
\t@echo "\\n======== Disassembly of start ========"
\t@objdump -d -j .text.boot $(KERNEL_ELF)

.PHONY: all kernel run run-debug debug clean clean-qemu memory-map
"""
        return makefile_content

    def run(self):
        self.find_files()
        makefile_content = self.generate_makefile()
        makefile_path = os.path.join(self.project_root, "Makefile")
        with open(makefile_path, "w") as f:
            f.write(makefile_content)
        print(f"generated makefile: {makefile_path}")

if __name__ == "__main__":
    project_root = os.getcwd()
    generator = MakefileGenerator(project_root)
    generator.run()
