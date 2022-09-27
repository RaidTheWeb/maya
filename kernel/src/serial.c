#include <serial.h>
#include <lib/io.h>
#include <lib/lock.h>

#define COM1 0x3f8
#define COM2 0x2f8
#define COM3 0x3e8
#define COM4 0x2e8

static spinlock_t serial_lock;

static int serial_initport(uint16_t port) {
    outb(port + 7, 0x69);
    if(inb(port + 7) != 0x69) return 0;

    outb(port + 1, 0x01);
    outb(port + 3, 0x80);

    outb(port + 0, 0x01);
    outb(port + 1, 0x00);

    outb(port + 3, 0x03);
    outb(port + 2, 0xc7);
    outb(port + 4, 0x0b);

    return 1;
}

static int serial_transitempty(uint16_t port) {
    return (inb(port + 5) & 0x40) != 0;
}

static void serial_transmit(uint16_t port, uint8_t val) {
    while(!serial_transitempty(port)) asm volatile ("pause"); // halt cpu function temporarily while the transit is full
    outb(port, val);
}

void serial_init(void) {
    serial_initport(COM1);
    serial_initport(COM2);
    serial_initport(COM3);
    serial_initport(COM4);
}

void serial_out(int c) {
    spinlock_acquire(&serial_lock);

    if(c == '\n') serial_transmit(COM1, '\r');
    serial_transmit(COM1, c);

    spinlock_release(&serial_lock);
}
