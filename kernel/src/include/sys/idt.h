#ifndef __SYS__IDT_H__
#define __SYS__IDT_H__

#include <stdint.h>

// from https://wiki.osdev.org/Interrupts
#define PITIRQ 0
#define KEYIRQ 1
#define COM2IRQ 3
#define COM1IRQ 4
#define LPT2IRQ 5
#define FLOPPYIRQ 6
#define LPT1IRQ 7
#define CMOSIRQ 8
#define MOUSEIRQ 12
#define FPUIRQ 13
#define ATAP 14
#define ATAS 15

extern void *isr[256];

void idt_updateist(uint8_t vec, uint8_t flags);
void idt_updateflags(uint8_t vec, uint8_t flags);

void idt_init(void);
uint8_t idt_allocvec(void);

#endif
