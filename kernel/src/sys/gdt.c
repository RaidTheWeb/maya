#include <sys/gdt.h>
#include <cpu/cpu.h>
#include <lib/lock.h>
#include <lib/stdio.h>

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint16_t baselow16;
    uint8_t basemid8;
    uint8_t access;
    uint8_t granularity;
    uint8_t basehigh8;
} gdtentry_t;

typedef struct __attribute__((packed)) {
    gdtentry_t entries[11]; // full gdt
} gdt_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} gdtptr_t;

static spinlock_t gdt_lock;
static gdt_t gdt;
static gdtptr_t gdtptr;

void gdt_init(void) {
    // null
    gdt.entries[0] = (gdtentry_t) {
        .limit = 0,
        .baselow16 = 0,
        .basemid8 = 0,
        .access = 0,
        .granularity = 0,
        .basehigh8 = 0
    };

    // ring 0 gdt entries for the limine terminal (16-bit, 32-bit and 64-bit)

    gdt.entries[1] = (gdtentry_t) { // ring 0 16 code
        .limit = 0xffff,
        .baselow16 = 0,
        .basemid8 = 0,
        .access = 0x9a,
        .granularity = 0,
        .basehigh8 = 0
    };

    gdt.entries[2] = (gdtentry_t) { // ring 0 16 data
        .limit = 0xffff,
        .baselow16 = 0,
        .basemid8 = 0,
        .access = 0x92,
        .granularity = 0,
        .basehigh8 = 0
    };

    gdt.entries[3] = (gdtentry_t) { // ring 0 32 code
        .limit = 0xffff,
        .baselow16 = 0,
        .basemid8 = 0,
        .access = 0x9a,
        .granularity = 0xcf,
        .basehigh8 = 0
    };

    gdt.entries[4] = (gdtentry_t) { // ring 0 32 data
        .limit = 0xffff,
        .baselow16 = 0,
        .basemid8 = 0,
        .access = 0x92,
        .granularity = 0xcf,
        .basehigh8 = 0
    };

    // actual standard gdt entries follow (kernel, user)

    gdt.entries[5] = (gdtentry_t) { // kernel 64 code
        .limit = 0,
        .baselow16 = 0,
        .basemid8 = 0,
        .access = 0x9a,
        .granularity = 0x20,
        .basehigh8 = 0
    };

    gdt.entries[6] = (gdtentry_t) { // kernel 64 data
        .limit = 0,
        .baselow16 = 0,
        .basemid8 = 0,
        .access = 0x92,
        .granularity = 0,
        .basehigh8 = 0
    };

    gdt.entries[7] = (gdtentry_t) { // user 64 code
        .limit = 0,
        .baselow16 = 0,
        .basemid8 = 0,
        .access = 0xf2,
        .granularity = 0,
        .basehigh8 = 0
    };

    gdt.entries[8] = (gdtentry_t) { // user 64 data
        .limit = 0,
        .baselow16 = 0,
        .basemid8 = 0,
        .access = 0xfa,
        .granularity = 0x20,
        .basehigh8 = 0
    };


    gdtptr.limit = sizeof(gdt_t) - 1;
    gdtptr.base = (uint64_t)&gdt;

    printf("[gdt]: GDT initialised\n");

    gdt_reload();
}

void gdt_reload(void) {
    asm volatile (
        "lgdt %0\n"
        "push rax\n"
        "push %1\n"
        "lea rax, [rip + 0x03]\n"
        "push rax\n"
        "retfq\n"
        "pop rax\n"
        "mov ds, %2\n"
        "mov es, %2\n"
        "mov ss, %2\n"
        "mov fs, %3\n"
        "mov gs, %3\n"
        : : "m" (gdtptr), "rm" ((uint64_t)0x28), "rm" ((uint32_t)0x30), "rm" ((uint32_t)0x3b)
        : "memory"
    );

    printf("[gdt]: GDT reloaded\n");
}
