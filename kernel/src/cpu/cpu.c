#include <limine.h>
#include <stddef.h>
#include <stdint.h>
#include <cpu/cpu.h>
#include <lib/stdio.h>

cpulocal_t *cpu_current(void) {
    if(cpu_intstate()) {
        printf("[cpu]: Attempted to get current cpu without interrupts disabled\n");
        for(;;) asm("hlt");
    }

    return 0; // we don't have a scheduler or anything, so it's always just 0 atm
}