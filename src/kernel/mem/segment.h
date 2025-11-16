#ifndef _MEM_SEGMENT_H
#define _MEM_SEGMENT_H

void gdt_build();
void gdt_install();
void gdt_dump();
void gdt_install_lgdt();
void gdt_install_jump();

#endif /* _MEM_SEGMENT_H */