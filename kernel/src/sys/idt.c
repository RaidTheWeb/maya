#include <stddef.h>
#include <sys/idt.h>
#include <lib/lock.h>
#include <lib/stdio.h>
#include <cpu/cpu.h>

typedef struct __attribute__((packed)) {
    uint16_t offlo16;
    uint16_t sel;
    uint8_t ist;
    uint8_t flags;
    uint16_t offmid16;
    uint32_t offhi32;
    uint32_t rsvd;
} idtentry_t;

typedef struct __attribute__((packed)) {
    uint16_t size;
    uint64_t addr;
} idtptr_t;

static spinlock_t idt_lock;
static uint8_t idt_freevec;

uint8_t idt_allocvec(void) {
    spinlock_acquire(&idt_lock);

    if(idt_freevec == 0xf0) {
        printf("[idt]: Failed to allocate vector as IDT registry is exhausted\n");
        return -1;
    }

    uint8_t ret = idt_freevec++;

    spinlock_release(&idt_lock);

    return ret;
}

static idtentry_t idt[256];
static idtptr_t idtptr;
extern void *int_thunks[];
void *isr[256];

static void isr_generic(uint32_t vec, void *unused) {
    (void)unused;
    printf("interrupt happened\n");
}

static void idt_reghandler(uint16_t vec, void *handler, uint8_t ist, uint8_t flags) {
    uint64_t handlerptr = (uint64_t)handler;

    idt[vec].offlo16 = (uint16_t)handlerptr;
    idt[vec].sel = 0x28; // kernel code segment
    idt[vec].ist = ist;
    idt[vec].flags = flags;
    idt[vec].offmid16 = (uint16_t)(handlerptr >> 16);
    idt[vec].offhi32 = (uint32_t)(handlerptr >> 32);
    idt[vec].rsvd = 0;
}

void idt_reload(void) {
    asm volatile (
        "lidt %0"
        : : "m"  (idtptr)
        : "memory"
    );
    printf("[idt]: IDT reloaded\n");
}

void idt_init(void) {

    idtptr = (idtptr_t) {
        .size = sizeof(idt) - 1,
        .addr = (uint64_t)idt
    };

    printf("[idt]: IDT initialised\n");
    for(uint64_t i = 0; i < 256; i++) {
        idt_reghandler(i, int_thunks[i], 0, 0x8e);
        isr[i] = isr_generic;
    }
    printf("[idt/isr]: Registered handlers\n");

    idt_reload();
}
