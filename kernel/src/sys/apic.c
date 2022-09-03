#include <sys/apic.h>
#include <lib/io.h>
#include <mm/mm.h>
#include <cpu/cpu.h>
#include <stdint.h>
#include <sys/pit.h>
#include <sys/madt.h>
#include <lib/stdio.h>

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

    uint64_t tick = pit_curcount();

    lapic_write(LAPIC_REGTIMERINIT, (uint32_t)samples);

    while(lapic_read(LAPIC_REGTIMERCUR) != 0);

    uint64_t final = pit_curcount();

    uint64_t total = tick - final;

    local->lapicfreq = (samples / total) * PIT_DIVEND;

    lapic_stoptimer();
}

void lapic_timeroneshot(cpulocal_t *local, uint8_t vec, uint64_t us) {
    lapic_stoptimer();

    uint64_t ticks = us * (local->lapicfreq / 1000000);
    
    lapic_write(LAPIC_REGLVTTIMER, vec);
    lapic_write(LAPIC_REGTIMERDIV, 0);
    lapic_write(LAPIC_REGTIMERINIT, (uint32_t)ticks);
}

void lapic_enable(uint8_t spur) {
    lapic_write(LAPIC_REGSPUR, lapic_read(LAPIC_REGSPUR) | (1 << 8) | spur);
}

void lapic_eoi(void) {
    lapic_write(LAPIC_REGEOI, 0);
}

void lapic_sendipi(uint8_t id, uint8_t vec) {
    lapic_write(LAPIC_REGICR1, (uint32_t)id << 24);
    lapic_write(LAPIC_REGICR0, vec);
}

uint32_t ioapic_read(int io, uint32_t reg)  {
    uint64_t base = (uint64_t)(LIST_ITEM(&madt_ioapics, io)->addr) + HIGHER_HALF;
    mmoutd(base, reg);
    return mmind(base + 16);
}

void ioapic_write(int io, uint32_t reg, uint32_t val) {
    uint64_t base = (uint64_t)(LIST_ITEM(&madt_ioapics, io)->addr) + HIGHER_HALF;
    mmoutd(base, reg);
    mmoutd(base + 16, val);
}

uint32_t ioapic_gsicount(int io) {
    return (ioapic_read(io, 1) & 0xff0000) >> 16;
}

int ioapic_fromgsi(uint32_t gsi) {
    for(uint64_t i = 0; i < madt_ioapics.length; i++) {
        if(gsi >= LIST_ITEM(&madt_ioapics, i)->gsib && gsi < LIST_ITEM(&madt_ioapics, i)->gsib + ioapic_gsicount(i)) return i; // we found the instance we wanted
    }

    printf("[apic]: Failed to determine IO APIC\n");
    for(;;) asm("hlt");
    return 0;
}

void ioapic_setgsiredirect(uint32_t id, uint8_t vec, uint32_t gsi, uint16_t flags, int status) {
    madtioapic_t *io = LIST_ITEM(&madt_ioapics, ioapic_fromgsi(gsi));

    uint64_t redirect = (uint64_t)vec;

    if((flags & (1 << 1)) != 0) redirect |= (1 << 13);
    if((flags & (1 << 3)) != 0) redirect |= (1 << 15);
    if(!status) redirect |= (1 << 16);

    redirect |= (uint64_t)id << 56;

    uint32_t iored = (gsi - io->gsib) * 2 + 16;
    ioapic_write(ioapic_fromgsi(gsi), iored, (uint32_t)redirect);
    ioapic_write(ioapic_fromgsi(gsi), iored + 1, (uint32_t)(redirect >> 32));
}

void ioapic_setirqredirect(uint32_t id, uint8_t vec, uint8_t irq, int status) { // redirect an IRQ to an interrupt vector
    for(uint64_t i = 0; i < madt_isos.length; i++) {
        madtiso_t *iso = LIST_ITEM(&madt_isos, i);
        if(iso->irqsrc != irq) continue;
        printf("[apic]: Using override on IRQ #%d (->%d)\n", irq, vec);
        ioapic_setgsiredirect(id, vec, iso->gsi, iso->flags, status);
        return;
    } 
    ioapic_setgsiredirect(id, vec, irq, 0, status);
}
