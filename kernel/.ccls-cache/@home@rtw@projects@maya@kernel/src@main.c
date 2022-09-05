#include <limine.h>
#include <stdint.h>
#include <stddef.h>
#include <mm/mm.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <lib/assert.h>
#include <lib/stdio.h>
#include <lib/string.h>
#include <serial.h>
#include <sys/idt.h>
#include <sys/gdt.h>
#include <sys/acpi.h>
#include <sys/apic.h>
#include <cpu/cpu.h>
#include <sys/time.h>
#include <dev/ps2.h>
#include <sys/pit.h>
#include <cpu/smp.h>

static volatile struct limine_terminal_request termreq = {
    .id = LIMINE_TERMINAL_REQUEST,
    .revision = 0
};

static volatile struct limine_hhdm_request hhdmreq = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

static volatile struct limine_memmap_request mmapreq = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

static volatile struct limine_rsdp_request rsdpreq = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

static volatile struct limine_smp_request smpreq = {
    .id = LIMINE_SMP_REQUEST,
    .revision = 0
};

static volatile struct limine_boot_time_request bootreq = {
    .id = LIMINE_BOOT_TIME_REQUEST,
    .revision = 0
};

static volatile struct limine_kernel_address_request kernreq = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

#define EXPECTEDHIGHERHALF 0xffff800000000000
uint64_t HIGHER_HALF = 0;

void termprint(const char *string, uint64_t len) {
    termreq.response->write(termreq.response->terminals[0], string, len);
}

void kmainthread(uint64_t arg) {
    printf("called via intercepted rip with argument 0x%x\n", arg);
    for(;;) asm("hlt"); // halt or else we'll try return somewhere else
}

// intercept stack frame and replace return address with that of a different function
void grabframeandreplaceret(uint64_t rsp) { 
    uint64_t *stack = (uint64_t *)rsp;
    stack[-1] = (uint64_t)kmainthread; // redirect function return to another location 
    asm volatile ( "mov rdi, 0x64" ); // give an argument to the function
}

void _start(void) {
    if(termreq.response == NULL || termreq.response->terminal_count < 1) for(;;) asm("hlt");
    if(hhdmreq.response == NULL) for(;;) asm("hlt");
    if(mmapreq.response == NULL || mmapreq.response->entry_count < 1) for(;;) asm("hlt");
    if(rsdpreq.response == NULL) {
        printf("[acpi]: ACPI is not supported\n");
        for(;;) asm("hlt");
    }
    if(smpreq.response == NULL) for(;;) asm("hlt");
    if(bootreq.response == NULL) for(;;) asm("hlt");
    if(kernreq.response == NULL) for(;;) asm("hlt");

    printf("[hhdm]: Verified HHDM offset: 0x%016llx\n", hhdmreq.response->offset);
    assert((hhdmreq.response->offset == EXPECTEDHIGHERHALF));
    HIGHER_HALF = hhdmreq.response->offset;
    printf("[hhdm]: HHDM offset matches expected higher half offset\n");

    pmm_init(mmapreq.response);

    serial_init();
    gdt_init();
    idt_init(); 

    vmm_init(mmapreq.response, kernreq.response);
 
    acpi_init(rsdpreq.response);

    smp_init(smpreq.response); 

    time_init(bootreq.response);

    ps2_init();

    char *buf = malloc(64);
    memcpy(buf, "Hello World!", 13);
    printf("%s\n", buf);

    // Scheduler notes:
    // Scheduling of threads is a requirement for maya to be able to make best use of available CPU cores (via SMP) to allow multiple things to run at any one time to allow things like background tasks (monitors, poll based device checks, etc.) and such.
    // CPU state can be ripped from stack when entering an IRQ (will be the LAPIC oneshot for the scheduler entry), as it pushes all the registers onto the stack as part of the state argument, rip is included within this state,
    // the scheduler could replace this rip value with a new one, so that when the interrupt goes to `iretq` it will return to a completely different location (thread entry) for execution to begin.
    // The thread state's rsp should point to a different location allocated directly with pmm_alloc() for the thread as not to cause any form of interference with the rest of the code (in the event of a stack overwrite of sorts)
    // Scheduler should run each thread on a timeslice, after setting up a thread to run a oneshot timer should be triggered so that the scheduler will reschedule the CPU after the timeslice has been completed, such that each thread gets to run for their own timeslice (timeslice should be allowed to be modified for cputime priority). Scheduling for now should just be a form of the Round Robin scheduling algorithm (https://wiki.osdev.org/Scheduling_Algorithms), some form of proper priority based algorithm can be considered in the future. 
    // The scheduler will follow preemptive scheduling as I believe that it'd be a good idea to do so. The base timeslice should be something around 50ms~ to maximise thread share time without impacting user experience. Blocking I/O should be considered to the scheduler as a good time to preempt the task.
    // Working with multiple processors could be tackled with a TLB cache design to reduce memory accesses when switching pagemaps. (as mentioned here https://wiki.osdev.org/Multiprocessor_Scheduling)


    // intercept stack frame (just experimenting with how a scheduler could be implemented)
    asm volatile (
        "mov rdi, rsp\n"
        "call grabframeandreplaceret\n"
    );

    for(;;) asm("hlt");
}

