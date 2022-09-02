#ifndef __LIB__IO_H__
#define __LIB__IO_H__

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %1, %0" : : "a" (val), "Nd" (port) : "memory" );
}

static inline void outw(uint16_t port, uint16_t val) {
    asm volatile ( "outw %1, %0" : : "a" (val), "Nd" (port) : "memory" );
}

static inline void outl(uint16_t port, uint32_t val) {
    asm volatile ( "outd %1, %0" : : "a" (val), "Nd" (port) : "memory" );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %0, %1" : "=a" (ret) : "Nd" (port) : "memory" );
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ( "inw %0, %1" : "=a" (ret) : "Nd" (port) : "memory" );
    return ret;
}

static inline uint32_t ind(uint16_t port) {
    uint32_t ret;
    asm volatile ( "ind %0, %1" : "=a" (ret) : "Nd" (port) : "memory" );
    return ret;
}

static inline uint8_t mminb(uint8_t addr) { 
    return *((volatile uint8_t *)addr);
}

static inline uint16_t mminw(uint16_t addr) { 
    return *((volatile uint16_t *)addr);
}

static inline uint32_t mmind(uint32_t addr) {
    return *((volatile uint32_t *)addr);
}

static inline uint64_t minq(uint64_t addr) {
    return *((volatile uint64_t *)addr);
}

static inline void mmoutb(uint8_t addr, uint8_t val) {
    (*((volatile uint8_t *)addr)) = val;
}

static inline void mmoutw(uint16_t addr, uint16_t val) {
    (*((volatile uint16_t *)addr)) = val;
}

static inline void mmoutd(uint32_t addr, uint32_t val) {
    (*((volatile uint32_t *)addr)) = val;
}

static inline void mmoutq(uint64_t addr, uint64_t val) {
    (*((volatile uint64_t *)addr)) = val;
}

// await I/O
#define iowait() ({ outb(0x80, 0x00); })

#endif
