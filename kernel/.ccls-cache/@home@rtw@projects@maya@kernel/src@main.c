#include <limine.h>
#include <stdint.h>
#include <stddef.h>
#include <mm/mm.h>
#include <mm/pmm.h>
#include <lib/assert.h>
#include <lib/stdio.h>
#include <lib/string.h>
#include <serial.h>

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

#define EXPECTEDHIGHERHALF 0xffff800000000000
uint64_t HIGHER_HALF = 0;

void termprint(const char *string, uint64_t len) {
    termreq.response->write(termreq.response->terminals[0], string, len);
}

void _start(void) {
    if(termreq.response == NULL || termreq.response->terminal_count < 1) for(;;) asm("hlt");
    if(hhdmreq.response == NULL) for(;;) asm("hlt");
    if(mmapreq.response == NULL || mmapreq.response->entry_count < 1) for(;;) asm("hlt");

    printf("[hhdm]: Verified HHDM offset: 0x%016llx\n", hhdmreq.response->offset);
    assert((hhdmreq.response->offset == EXPECTEDHIGHERHALF));
    HIGHER_HALF = hhdmreq.response->offset;
    printf("[hhdm]: HHDM offset matches expected higher half offset\n");

    serial_init();

    pmm_init(mmapreq.response);

    char *buf = malloc(64);
    memcpy(buf, "Hello World!", 13);
    printf("%s\n", buf);

    for(;;) asm("hlt");
}

