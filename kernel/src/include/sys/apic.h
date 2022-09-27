#ifndef __SYS__LAPIC_H__
#define __SYS__LAPIC_H__

#include <cpu/cpu.h>

#define LAPIC_REGID 0x020
#define LAPIC_REGEOI 0x0b0
#define LAPIC_REGSPUR 0xf0
#define LAPIC_REGICR0 0x300
#define LAPIC_REGICR1 0x310
#define LAPIC_REGLVTTIMER 0x320
#define LAPIC_REGTIMERINIT 0x380
#define LAPIC_REGTIMERCUR 0x390
#define LAPIC_REGTIMERDIV 0x3e0

extern uint64_t lapic_base;

/**
 * Local Advanced Programmble Interrupt Controller read
 *
 * @param reg LAPIC register
 * @returns Register value
 */
uint32_t lapic_read(uint32_t reg);
/**
 * Local Advanced Programmble Interrupt Controller write
 *
 * @param reg LAPIC register
 * @param val Value to write
 */
void lapic_write(uint32_t reg, uint32_t val);
/**
 * Stop LAPIC timer (per CPU)
 *
 */
void lapic_stoptimer(void);
/**
 * Calibrate LAPIC timer (per CPU)
 *
 * @param local CPU local instance
 */
void lapic_calibratetimer(cpulocal_t *local);
/**
 * Launch a oneshot timer on LAPIC (per CPU)
 *
 * @param local CPU local instance
 * @param vec IPI vector
 * @param us Microseconds to run for
 */
void lapic_timeroneshot(cpulocal_t *local, uint8_t vec, uint64_t us);
/**
 * Enable LAPIC
 *
 * @param vec Spurious vector
 */
void lapic_enable(uint8_t vec);
/**
 * Trigger LAPIC EOI
 *
 */
void lapic_eoi(void);
/**
 * Send Inter-Process Interrupt
 *
 * @param id LAPIC ID
 * @param vec IPI vector
 */
void lapic_sendipi(uint8_t id, uint8_t vec);

/**
 * Read IO APIC Register
 *
 * @param io IO APIC
 * @param reg IO APIC register
 * @returns Register value
 */
uint32_t ioapic_read(int io, uint32_t reg);
/**
 * Write IO APIC Register
 *
 * @param io IO APIC
 * @param reg IO APIC register
 * @param val Value to write
 */
void ioapic_write(int io, uint32_t reg, uint32_t val);
/**
 * Get IO APIC GSI Count
 *
 * @param io IO APIC
 * @returns GSI count
 */
uint32_t ioapic_gsicount(int io);
/**
 * Get IO APIC from GSI
 *
 * @param gsi GSI
 * @returns IO APIC
 */
int ioapic_fromgsi(uint32_t gsi);
/**
 * Set a GSI redirect on IO APIC
 *
 * @param id LAPIC ID
 * @param vec Vector
 * @param gsi GSI to redirect
 * @param flags Redirection flags
 * @param status GSI status
 */
void ioapic_setgsiredirect(uint32_t id, uint8_t vec, uint32_t gsi, uint16_t flags, int status);
/**
 * Set an IRQ redirect on IO APIC
 *
 * @param id LAPIC ID
 * @param vec Vector
 * @param irq IRQ to redirect
 * @param status IRQ status
 */
void ioapic_setirqredirect(uint32_t id, uint8_t vec, uint8_t irq, int status);

#endif
