#include <dev/ps2.h>
#include <sys/idt.h>
#include <sys/apic.h>
#include <lib/io.h>
#include <lib/stdio.h>
#include <mm/mm.h>
#include <mm/vmm.h>

static uint8_t ps2vec = 0;

uint8_t ps2_read(void) {
    while ((inb(0x64) & 1) == 0); // locked read
    return inb(0x60);
}

void ps2_write(uint16_t port, uint8_t val) {
    while ((inb(0x64) & 2) != 0); // locked write
    outb(port, val);
}

uint8_t ps2_readcfg(void) {
    ps2_write(0x64, 0x20); // ask for config
    return ps2_read();
}

void ps2_writecfg(uint8_t val) {
    ps2_write(0x64, 0x60); // ask to write to config
    ps2_write(0x60, val); // write config
}

static void ps2_handler(uint32_t vec, cpustate_t *state) {
    (void)vec;
    (void)state;
    lapic_eoi();
    uint8_t scancode = inb(0x60);
    printf("0x%08x ", scancode);
}

void ps2_init(void) {
    ps2_write(0x64, 0xad);
    ps2_write(0x64, 0xa7);

    uint8_t config = ps2_readcfg();
    config |= (1 << 0) | (1 << 6); // enable keyboard translation + interrupts
    if((config & (1 << 5)) != 0) config |= (1 << 1); // enable mouse (if we have one)
    ps2_writecfg(config);

    ps2_write(0x64, 0xae);
    if((config & (1 << 5)) != 0) ps2_write(0x64, 0xa8);

    ps2vec = idt_allocvec();
    isr[ps2vec] = ps2_handler;
    printf("setting up on %d\n", LIST_ITEM(&cpulocals, 0)->lapicid);
    ioapic_setirqredirect(LIST_ITEM(&cpulocals, 0)->lapicid, ps2vec, 1, 1); // we just redirect with lapic id 0 as CPU 0 can do all the work (independant of SMP configuration)

    printf("[ps2]: PS/2 vector: %d\n", ps2vec);
    inb(0x60); // acknowledge
    printf("[ps2]: PS/2 initialised\n");
}
