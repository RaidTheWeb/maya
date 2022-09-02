#ifndef __SYS__GDT_H__
#define __SYS__GDT_H__

#include <cpu/cpu.h>

void gdt_init(void);
void gdt_reload(void);
void gdt_loadtss(tss_t *tss);

#endif
