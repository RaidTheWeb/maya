#include <lib/stdio.h>
#include <sys/idt.h>
#include <sys/panic.h>

void panic(cpustate_t *state, char *message) {
    cpu_disableints();
    LIST_FOR_EACH(&cpulocals, local,
        cpulocal_t *cpu = *local;
        if(cpu_current()->lapicid == cpu->lapicid) continue;
        lapic_sendipi(cpu->lapicid, abortvec);
        while(!cpu->aborted) asm("pause");
    );

    int cpunum = cpu_current()->cpunum;

    printf_panic("Kernel Panic on CPU #%d with \"%s\"\n", cpunum, message);
    for(;;) asm("hlt");
}
