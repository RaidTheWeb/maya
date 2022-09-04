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

void kmainthread(void) {
    printf("A\n");
    for(;;) asm("hlt");
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

    for(;;) asm("hlt");
}

