import os
import glob
import sys
from typing import List, Dict, Set, Tuple, Optional

class MakefileGenerator:
    def __init__(self, project_root: str):
        self.project_root = project_root
        self.build_dir = "boot"
        self.loader_dir = "loader"
        self.kernel_dir = "kernel"
        self.mem_dir = "mem"
        self.fix_linker_script = True
        self.arch_dir = f"{self.kernel_dir}/arch"
        self.cflags = "-m64 -ffreestanding -fno-pic -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2"
        self.includes = f"-I{self.kernel_dir} -I{self.kernel_dir}/util -I{self.mem_dir}"
        self.debug_flags = "-g -O0"
        self.img = f"{self.build_dir}/litecore.img"
        self.boot_bin = f"{self.build_dir}/boot.bin"
        self.kernel_bin = f"{self.build_dir}/kernel.bin"
        self.kernel_elf = f"{self.build_dir}/kernel.elf"
        self.asm_files = []
        self.c_files = []
        self.h_files = []

    def find_files(self):
        for root, _, files in os.walk(self.project_root):
            if ".git" in root:
                continue
            for file in files:
                full_path = os.path.join(root, file)
                rel_path = os.path.relpath(full_path, self.project_root)
                if file.endswith(".c"):
                    self.c_files.append(rel_path)
                elif file.endswith(".h"):
                    self.h_files.append(rel_path)
                elif file.endswith(".asm"):
                    self.asm_files.append(rel_path)

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

    def check_and_fix_linker_script(self) -> Optional[str]:
        linker_script_path = os.path.join(self.project_root, self.kernel_dir, "linker.ld")
        if not os.path.exists(linker_script_path) or not self.fix_linker_script:
            return None
            
        backup_path = f"{linker_script_path}.bak"
        if not os.path.exists(backup_path):
            try:
                with open(linker_script_path, 'r') as f:
                    original_content = f.read()
                with open(backup_path, 'w') as f:
                    f.write(original_content)
                print(f"Backup created: {backup_path}")
            except Exception as e:
                print(f"Warning: Error during linker script backup: {e}")
                return None
        
        # Create new linker script with correct format
        new_linker_script = """ENTRY(_start)

PHDRS
{
  text PT_LOAD FLAGS(5); /* r-x */
  rodata PT_LOAD FLAGS(4); /* r-- */
  data PT_LOAD FLAGS(6); /* rw- */
}

SECTIONS
{
    . = 0x1000;
    
    .text.boot ALIGN(16) : {
        *(.text.boot)
    } :text
    
    .text ALIGN(16) : {
        *(.text)
        *(.text.*)
    } :text
    
    .rodata ALIGN(16) : {
        *(.rodata)
        *(.rodata.*)
        *(.eh_frame)
        *(.rela.dyn)
    } :rodata
    
    .data ALIGN(16) : {
        *(.data)
        *(.data.*)
        *(.got)
        *(.got.plt)
        *(.igot.plt)
    } :data
    
    .bss ALIGN(16) : {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    } :data
}
"""
        
        try:
            with open(linker_script_path, 'w') as f:
                f.write(new_linker_script)
            return linker_script_path
        except Exception as e:
            print(f"Warn: linkerscript fixing err: {e}")
            return None

    def generate_makefile(self) -> str:
        kernel_start_asm = next((f for f in self.asm_files if f.endswith("start.asm") and self.kernel_dir in f), "")
        loader_asm = next((f for f in self.asm_files if f.endswith("boot.asm") and self.loader_dir in f), "")
        other_asm_files = [f for f in self.asm_files 
                          if f != kernel_start_asm and 
                             f != loader_asm and
                             (f.startswith(self.kernel_dir) or f.startswith(self.mem_dir))]
        kernel_c_srcs = [f for f in self.c_files if f.startswith(self.kernel_dir) or f.startswith(self.mem_dir)]
        kernel_c_srcs_var = " ".join(kernel_c_srcs)
        kernel_objs = []
        for src in kernel_c_srcs:
            obj_name = os.path.basename(src).replace(".c", ".o")
            kernel_objs.append(f"{self.build_dir}/{obj_name}")
        if kernel_start_asm:
            kernel_objs.insert(0, f"{self.build_dir}/start.o")
        for asm_file in other_asm_files:
            obj_name = os.path.basename(asm_file).replace(".asm", ".o")
            obj_path = f"{self.build_dir}/{obj_name}"
            if obj_path not in kernel_objs:
                kernel_objs.append(obj_path)
        kernel_objs_var = " ".join(kernel_objs)
        other_asm_files_var = " ".join(other_asm_files)
        makefile_content = f"""LOADER_DIR = {self.loader_dir}
KERNEL_DIR = {self.kernel_dir}
MEM_DIR = {self.mem_dir}
ARCH_DIR = $(KERNEL_DIR)/arch
BUILD_DIR = {self.build_dir}

LOADER_ASM = {loader_asm}
KERNEL_START_ASM = {kernel_start_asm}
KERNEL_C_SRCS = {kernel_c_srcs_var}
OTHER_ASM_SRCS = {other_asm_files_var}
KERNEL_OBJS = {kernel_objs_var}

IMG = $(BUILD_DIR)/litecore.img
BOOT_BIN = $(BUILD_DIR)/boot.bin
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
KERNEL_ELF = $(BUILD_DIR)/kernel.elf

VERSION = $(shell cat version.txt)
CFLAGS = {self.cflags} {self.includes} {self.debug_flags} -DKERNEL_VERSION=\\"$(VERSION)\\"

all: $(IMG)

$(BOOT_BIN): $(LOADER_ASM)
\tmkdir -p $(BUILD_DIR)
\tnasm -f bin -o $@ $<

$(BUILD_DIR)/start.o: $(KERNEL_START_ASM)
\tmkdir -p $(BUILD_DIR)
\tnasm -f elf64 -g -F dwarf -o $@ $< -w-gnu-stack

"""
        for asm_file in other_asm_files:
            obj_name = os.path.basename(asm_file).replace(".asm", ".o")
            obj_path = f"$(BUILD_DIR)/{obj_name}"
            makefile_content += f"{obj_path}: {asm_file}\n"
            makefile_content += "\tmkdir -p $(BUILD_DIR)\n"
            if "arch" in asm_file or "mode_switch" in asm_file:
                makefile_content += "\tnasm -f elf64 -g -F dwarf -o $@ $< -w-gnu-stack\n\n"
            else:
                makefile_content += "\tnasm -f elf64 -g -F dwarf -o $@ $< -w-gnu-stack\n\n"
        for c_file in kernel_c_srcs:
            obj_name = os.path.basename(c_file).replace(".c", ".o")
            obj_path = f"$(BUILD_DIR)/{obj_name}"
            deps = self.get_dependencies(c_file)
            deps_str = " ".join(deps) if deps else ""
            makefile_content += f"{obj_path}: {c_file} {deps_str}\n"
            makefile_content += "\tmkdir -p $(BUILD_DIR)\n"
            
            if "arch/arch.c" in c_file:
                arch_cflags = f"-m32 -ffreestanding -fno-pic {self.includes} {self.debug_flags} -DKERNEL_VERSION=\\\"$(VERSION)\\\" -mno-red-zone"
                makefile_content += f"\tgcc {arch_cflags} -c $< -o $@\n\n"
            else:
                makefile_content += "\tgcc $(CFLAGS) -c $< -o $@\n\n"
        makefile_content += """$(BUILD_DIR)/arch_boot.o: $(BUILD_DIR)/arch.o
\tobjcopy -O elf64-x86-64 -B i386:x86-64 $(BUILD_DIR)/arch.o $(BUILD_DIR)/arch_boot.o

$(KERNEL_BIN): $(filter-out $(BUILD_DIR)/arch.o,$(KERNEL_OBJS)) $(BUILD_DIR)/arch_boot.o
\tld -m elf_x86_64 -T $(KERNEL_DIR)/linker.ld -o $(KERNEL_ELF) --build-id=none -g $^ -z noexecstack
\tobjcopy -O binary $(KERNEL_ELF) $@
\t@echo "\\033[0;32mKernel size: $$(wc -c < $@) bytes\\033[0m"
\t@echo "entry: 0x1000(16bit mode)"

$(IMG): $(BOOT_BIN) $(KERNEL_BIN)
\tcat $(BOOT_BIN) $(KERNEL_BIN) > $(IMG)
\ttruncate -s 10M $(IMG)

run: $(IMG)
\tqemu-system-x86_64 -drive file=$(IMG),format=raw -monitor stdio -cpu qemu64 -no-reboot -no-shutdown

run-debug: $(IMG)
\tqemu-system-x86_64 -drive file=$(IMG),format=raw -monitor stdio -cpu qemu64 -no-reboot -no-shutdown -d int,cpu_reset,exec,in_asm,page -D qemu.log

debug: $(IMG) clean-qemu
\t@echo "Starting QEMU with GDB server on port 1234..."
\tqemu-system-x86_64 -drive file=$(IMG),format=raw -s -S -cpu qemu64 -no-reboot -no-shutdown &
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

.PHONY: all clean run run-debug debug memory-map
"""
        return makefile_content

    def run(self):
        self.find_files()
        
        fixed_script = self.check_and_fix_linker_script()
        if fixed_script:
            print(f"fixed linker script: {fixed_script}")
        
        makefile_content = self.generate_makefile()
        makefile_path = os.path.join(self.project_root, "Makefile")
        with open(makefile_path, "w") as f:
            f.write(makefile_content)
        
        print(f"generated makefile: {makefile_path}")

if __name__ == "__main__":
    project_root = os.getcwd()
    generator = MakefileGenerator(project_root)
    generator.run()
