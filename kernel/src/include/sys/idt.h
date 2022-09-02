#ifndef __SYS__IDT_H__
#define __SYS__IDT_H__

#include <stdint.h>

void idt_init(void);

uint8_t idt_allocvec(void);

#endif
