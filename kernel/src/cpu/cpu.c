#include <limine.h>
#include <stddef.h>
#include <stdint.h>
#include <cpu/cpu.h>
#include <lib/stdio.h>

typeof(cpulocals) cpulocals = LIST_INIT;

cpulocal_t *cpu_current(void) {
    if(cpu_intstate()) {
        printf("[cpu]: Attempted to get current cpu without interrupts disabled\n");
        for(;;) asm("hlt");
    }

    return cpu_getgsbase(); // grab the cpu information
}
