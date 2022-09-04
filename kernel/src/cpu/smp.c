#include <cpu/cpu.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/gdt.h>
#include <sys/idt.h>
#include <mm/mm.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <lib/stdio.h>

#define CPUSTACK 0x10000

uint64_t cpucount = 0;

void smp_initcpu(struct limine_smp_info *info) {
    cpulocal_t *local = (cpulocal_t *)info->extra_argument;
    int cpunum = local->cpunum;

    local->lapicid = info->lapic_id;

    gdt_reload();
    idt_reload();

    gdt_loadtss(&local->tss);

    vmm_switchto(kernelpagemap);

    cpu_setgsbase(local); // put local into CPU "global segment"

    uint64_t *intstackphys = pmm_alloc(CPUSTACK / PAGE_SIZE); // allocate pages for stack (about 10)
    if(intstackphys == NULL) {
        printf("[smp]: Failed to allocate interrupt stack\n");
        for(;;) asm("hlt");
    }
    local->tss.rsp0 = (uint64_t)((void *)intstackphys + CPUSTACK + HIGHER_HALF);
    uint64_t *schedstackphys = pmm_alloc(CPUSTACK / PAGE_SIZE);
    if(schedstackphys == NULL) {
        printf("[smp]: Failed to allocate scheduler stack\n");
        for(;;) asm("hlt");
    }
    local->tss.ist1 = (uint64_t)((void *)schedstackphys + CPUSTACK + HIGHER_HALF);

    // initialise features

    cpu_toggleint(1); // enable interrupts on this CPU
    printf("[smp]: Processor #%d online\n", cpunum);

    cpucount++;
    if(cpunum != 0) while(1) asm("pause"); // wait until we're given a task (cpu 0 will fallback since it still has to initialise the main loop)
}

// CPU 0 is tasked with the bulk of the work during early initialsation
// other CPUs on an SMP system will allow the other CPUs to do work (CPU 0 handles interrupts, kernel initialisation before handing off as a scheduled task (so that it may keep priority on handling interrupts rather than on keeping the kernel running))

// TODO: Proper initialisation of FPU and SSE

void smp_init(struct limine_smp_response *smp) {
    printf("[smp]: Detected %d processors\n", smp->cpu_count);

    for(uint64_t i = 0; i < smp->cpu_count; i++) {
        cpulocal_t *local = malloc(sizeof(cpulocal_t));
        LIST_PUSHBACK(&cpulocals, local);

        smp->cpus[i]->extra_argument = (uint64_t)local;
        local->cpunum = i;

        printf("[smp]: Setting up processor #%d\n", i);

        if(smp->cpus[i]->lapic_id == smp->bsp_lapic_id) {
            smp_initcpu(smp->cpus[i]); // initialise the base CPU off the bat (it'll run before the initialisation of anything else)
        } else {
            smp->cpus[i]->goto_address = smp_initcpu; // preemptively task CPUs to initialise themselves 
        }
    }

    while(cpucount != smp->cpu_count) asm("pause"); // await all starting
    printf("[smp]: SMP initialised on %d processors\n", cpucount);
}
