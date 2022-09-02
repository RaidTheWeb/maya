#include <sys/apic.h>
#include <lib/io.h>
#include <mm/mm.h>
#include <cpu/cpu.h>
#include <stdint.h>

uint64_t lapic_base = 0;

uint32_t lapic_read(uint32_t reg) {
    if(!lapic_base) lapic_base = (uint64_t)(msr_read(0x1b) & 0xfffff000) + HIGHER_HALF;
    return mmind(lapic_base + reg);
}

void lapic_write(uint32_t reg, uint32_t val) {
    if(!lapic_base) lapic_base = (uint64_t)(msr_read(0x1b) & 0xfffff000) + HIGHER_HALF;
    mmoutd(lapic_base + reg, val);
}

void lapic_stoptimer(void) {
    lapic_write(LAPIC_REGTIMERINIT, 0);
    lapic_write(LAPIC_REGLVTTIMER, (1 << 16));
}

void lapic_calibratetimer(cpulocal_t *local) {
    lapic_stoptimer();

    uint64_t samples = 0xffff;

    lapic_write(LAPIC_REGLVTTIMER, (1 << 16) | 0xff);
    lapic_write(LAPIC_REGTIMERDIV, 0);

    // uint64_t tick = 
}
