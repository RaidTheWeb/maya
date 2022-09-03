#include <sys/pit.h>
#include <cpu/cpu.h>
#include <lib/io.h>
#include <sys/time.h>
#include <sys/apic.h>
#include <sys/idt.h>
#include <lib/stdio.h>

uint16_t pit_curcount(void) {
    outb(0x43, 0x00);
    uint8_t lo = inb(0x40);
    uint8_t hi = inb(0x40) << 8;
    return ((uint16_t)hi << 8) | lo;
}

void pit_setreload(uint16_t count) {
    outb(0x43, 0x34);
    outb(0x40, (uint8_t)count);
    outb(0x40, (uint8_t)(count >> 8));
}

void pit_setfreq(uint64_t freq) {
    uint64_t newdiv = PIT_DIVEND / freq;
    if(PIT_DIVEND % freq > freq / 2) newdiv++;
    pit_setreload((uint16_t)newdiv);
}

static void pit_handler(uint32_t num, cpulocal_t *state) {
    printf("perhaps a pit\n");
    // timer handler
    lapic_eoi();
}

void pit_init(void) {
    pit_setfreq(TIMERFREQ);
    
    uint8_t vec = idt_allocvec(); // get us a vector to use for out PIT
    isr[vec] = pit_handler;
    ioapic_setirqredirect(0, vec, 0, 1);
    printf("[pit]: PIT initialised\n");
}
