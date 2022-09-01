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
    asm volatile ( "inb %0, %1" : "=a" (ret) : "Nd" (port) : "memory" );
    return ret;
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile ( "inb %0, %1" : "=a" (ret) : "Nd" (port) : "memory" );
    return ret;
}

#endif
