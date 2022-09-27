#ifndef __SYS__PANIC_H__
#define __SYS__PANIC_H__

#include <cpu/cpu.h>
#include <lib/list.h>
#include <sys/apic.h>

void panic(cpustate_t *state, char *message);

#endif
