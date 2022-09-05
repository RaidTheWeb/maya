#include <cpu/cpu.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/gdt.h>
#include <sys/idt.h>
#include <mm/mm.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <lib/stdio.h>
#include <sys/apic.h>
#include <proc/proc.h>

#define CPUSTACK 0x10000

uint64_t cpucount = 0;

uint64_t fpu_storesize = 0;
void (*fpu_save)(void *ctx) = NULL;
void (*fpu_restore)(void *ctx) = NULL;

void smp_initcpu(struct limine_smp_info *info) {
    cpulocal_t *local = (cpulocal_t *)info->extra_argument;
    int cpunum = local->cpunum;

    local->lapicid = info->lapic_id;

    gdt_reload();
    idt_reload();

    gdt_loadtss(&local->tss);

    vmm_switchto(kernelpagemap);

    cpu_setgsbase(local);

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

    // enable syscall for amd/intel systems
    // uint64_t efer = msr_read(0xc0000080);
    // efer |= 1; // enable syscall
    // msr_write(0xc0000080, efer);
    // msr_write(0xc0000081, 0x0033002800000000); // set up the funny

    // msr_write(0xc0000082, syscall_entry);
    // msr_write(0xc0000084, (uint64_t)(~((uint64_t)0x002)));

    // enable PAT
    uint64_t pat = msr_read(0x277);
    pat &= 0xffffffff;
    pat |= (uint64_t)0x0105 << 32;
    msr_write(0x277, pat);

    // enable SSE and SSE2 so we can anable SIMD (Single Instruction Multiple Data)
    uint64_t cr0 = cpu_readcr0();
    cr0 &= ~(1 << 2);
    cr0 |= (1 << 1);
    cpu_writecr0(cr0);

    uint64_t cr4 = cpu_readcr4();
    cr4 |= (3 << 9);
    cpu_writecr4(cr4);

    uint32_t eax, ebx, ecx, edx;

    if(cpuid(1, 0, &eax, &ebx, &ecx, &edx) && (ecx & CPUID_XSAVE)) {
        if(cpunum == 0) { // only print on CPU0 to cut down on reprints
            printf("[fpu]: XSAVE supported\n");
        }

        // enable XSAVE
        cr4 = cpu_readcr4();
        cr4 |= (uint64_t)1 << 18;
        cpu_writecr4(cr4);

        uint64_t xcr0 = 0;
        if(cpunum == 0) printf("[fpu]: Saving x87 state using XSAVE on all processors\n");
        xcr0 |= (uint64_t)1 << 0;
        if(cpunum == 0) printf("[fpu]: Saving SSE state using XSAVE on all processors\n");
        xcr0 |= (uint64_t)1 << 1;

        if(ecx & CPUID_AVX) {
            if(cpunum == 0) printf("[fpu]: Saving AVX state using XSAVE on all processors\n");
            xcr0 |= (uint64_t)1 << 2;
        }

        if(cpuid(7, 0, &eax, &ebx, &ecx, &edx) && (ebx & CPUID_AVX512)) {
            if(cpunum == 0) printf("[fpu]: Saving AVX512 state using XSAVE on all processors\n");
            xcr0 |= (uint64_t)1 << 5;
            xcr0 |= (uint64_t)1 << 6;
            xcr0 |= (uint64_t)1 << 7;
        }

        cpu_wrxcr(0, xcr0);

        if(!cpuid(13, 0, &eax, &ebx, &ecx, &edx)) {
            printf("[smp]: CPUID failed\n");
            for(;;) asm("hlt");
        }

        fpu_storesize = ecx;
        fpu_save = cpu_xsave;
        fpu_restore = cpu_xrstor;
    } else {
        if(cpunum == 0) printf("[fpu]: Initialising legacy FXSAVE on all processors\n");
        fpu_storesize = 512;
        fpu_save = cpu_fxsave;
        fpu_restore = cpu_fxrstor;
    }


    lapic_enable(0xff);

    cpu_toggleint(1); // enable interrupts on this CPU
    
    // lapic_calibratetimer(local);
    printf("[smp]: Processor #%d online\n", cpunum);

    // setting a cpu's state for rip will redirect where it needs to go

    cpucount++;
    if(cpunum != 0) for(;;) asm("hlt"); // wait until we're given a task (cpu 0 will fallback since it still has to initialise the main loop)
}

// CPU 0 is tasked with the bulk of the work during early initialsation
// other CPUs on an SMP system will allow the other CPUs to do work (CPU 0 handles interrupts, kernel initialisation before handing off as a scheduled task (so that it may keep priority on handling interrupts rather than on keeping the kernel running))

// TODO: Proper initialisation of FPU and SSE

void smp_init(struct limine_smp_response *smp) {
    printf("[smp]: Detected %d processors\n", smp->cpu_count);

    for(uint64_t i = 0; i < smp->cpu_count; i++) {
        cpulocal_t *local = malloc(sizeof(cpulocal_t));

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
    printf("[smp]: SMP initialised on all %d processors\n", cpucount);
}
