#ifndef __CPU__CPU_H__
#define __CPU__CPU_H__

#include <stdint.h>
#include <lib/list.h>

typedef struct {
    uint64_t ds; // Data segment
    uint64_t es; // Extra segment
    uint64_t rax; // Accumulator
    uint64_t rbx; // Base
    uint64_t rcx; // Counter
    uint64_t rdx; // Data
    uint64_t rsi; // Source index
    uint64_t rdi; // Destination index
    uint64_t rbp; // Base pointer
    uint64_t r8; // Generic
    uint64_t r9; // Generic
    uint64_t r10; // Generic
    uint64_t r11; // Generic
    uint64_t r12; // Generic
    uint64_t r13; // Generic
    uint64_t r14; // Generic
    uint64_t r15; // Generic
    uint64_t err; // Current error code
    uint64_t rip; // Instruction pointer
    uint64_t cs; // Code segment
    uint64_t rflags; // Flags 
    uint64_t rsp; // Stack pointer
    uint64_t ss; // Stack segment
} cpustate_t;

typedef struct __attribute__((packed)) {
    uint32_t rsvd0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t rsvd1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t rsvd2;
    uint32_t iopb;
} tss_t;

// cpu local will contain references to current processes, threads, etc. (might be better to seperate them though and timeshare that way)

typedef struct {
    int cpunum;
    int active;
    int lastrunqueue;
    uint32_t lapicid;
    uint32_t lapicfreq;
    tss_t tss;
    void (*timerfunc)(int, cpustate_t *);
} cpulocal_t;

cpulocal_t *cpu_current(void);
void cpu_init(void);

static inline uint64_t cpu_readcr0(void) {
    uint64_t ret = 0;
    asm volatile (
        "mov %0, cr0"
        : "=r" (ret)
        : : "memory"
    );
    return ret;
}

static inline void cpu_writecr0(uint64_t val) {
    asm volatile (
        "mov cr0, %0"
        : : "r" (val)
        : "memory"
    );
}

static inline uint64_t cpu_readcr1(void) {
    uint64_t ret = 0;
    asm volatile (
        "mov %0, cr1"
        : "=r" (ret)
        : : "memory"
    );
    return ret;
}

static inline void cpu_writecr1(uint64_t val) {
    asm volatile (
        "mov cr1, %0"
        : : "r" (val)
        : "memory"
    );
}

static inline uint64_t cpu_readcr2(void) {
    uint64_t ret = 0;
    asm volatile (
        "mov %0, cr2"
        : "=r" (ret)
        : : "memory"
    );
    return ret;
}

static inline void cpu_writecr2(uint64_t val) {
    asm volatile (
        "mov cr2, %0"
        : : "r" (val)
        : "memory"
    );
}

static inline uint64_t cpu_readcr3(void) {
    uint64_t ret = 0;
    asm volatile (
        "mov %0, cr3"
        : "=r" (ret)
        : : "memory"
    );
    return ret;
}

static inline void cpu_writecr3(uint64_t val) {
    asm volatile (
        "mov cr3, %0"
        : : "r" (val)
        : "memory"
    );
}

static inline uint64_t cpu_readcr4(void) {
    uint64_t ret = 0;
    asm volatile (
        "mov %0, cr4"
        : "=r" (ret)
        : : "memory"
    );
    return ret;
}

static inline void cpu_writecr4(uint64_t val) {
    asm volatile (
        "mov cr4, %0"
        : : "r" (val)
        : "memory"
    );
}

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t edx = 0, eax = 0;
    asm volatile (
        "rdmsr"
        : "=a" (eax), "=d" (edx)
        : "c" (msr)
        : "memory"
    );
    return ((uint64_t)edx << 32) | eax;
}

static inline uint64_t wrmsr(uint32_t msr, uint64_t val) {
    uint32_t eax = (uint32_t)val, edx = (uint32_t)(val >> 32);
    asm volatile (
        "wrmsr"
        : : "a" (eax), "d" (edx), "c" (msr)
        : "memory"
    );
    return ((uint64_t)edx << 32) | eax;
}

static inline void cpu_setkerngsbase(void *addr) {
    wrmsr(0xc0000102, (uint64_t)addr);
}

static inline void cpu_setgsbase(void *addr) {
    wrmsr(0xc0000101, (uint64_t)addr);
}

static inline void cpu_setfsbase(void *addr) {
    wrmsr(0xc0000100, (uint64_t)addr);
}

static inline void *cpu_getkerngsbase(void) {
    return (void *)rdmsr(0xc0000102);
}

static inline void *cpu_getgsbase(void) {
    return (void *)rdmsr(0xc0000101);
}

static inline void *cpu_getfsbase(void) {
    return (void *)rdmsr(0xc0000100);
}

static inline void cpu_invlpg(uint64_t val) {
    asm volatile (
        "invlpg [%0]"
        : : "r" (val)
        : "memory"
    );
}

static inline void cpu_enableints(void) {
    asm("sti");
}

static inline void cpu_disableints(void) {
    asm("cli");
}

static inline int cpu_intstate(void) {
    uint64_t f;
    asm volatile ( "pushfq\npop %0" : "=rm" (f) ); // push flags onto stack and pop off into flags variable
    return f & (1 << 9); // grab the interrupt state flag
}

static inline uint64_t msr_read(uint32_t msr) {
    uint32_t eax = 0, edx = 0;
    asm volatile (
        "rdmsr"
        : "=a" (eax), "=d" (edx)
        : "c" (msr)
        : "memory"
    );
    return ((uint64_t)edx << 32) | eax;
}

static inline void msr_write(uint32_t msr, uint64_t val) {
    uint32_t eax = val, edx = val >> 32;
    asm volatile (
        "wrmsr"
        : : "a" (eax), "d" (edx), "c" (msr)
        : "memory"
    );
}

static inline void cpu_wrxcr(uint32_t reg, uint64_t val) {
    uint32_t a = val, d = val >> 32;
    asm volatile ( "xsetbv" :: "a" (a), "d" (d), "c" (reg) : "memory" );
}

static inline void cpu_xsave(void *ctx) {
    asm volatile ( "xsave [%0]" : : "r" (ctx), "a" (0xffffffff), "d" (0xffffffff) : "memory" );
}

static inline void cpu_xrstor(void *ctx) {
    asm volatile ( "xrstor [%0]" : : "r" (ctx), "a" (0xffffffff), "d" (0xffffffff) : "memory" );
}

static inline void cpu_fxsave(void *ctx) {
    asm volatile ( "fxsave [%0]" : : "r" (ctx) : "memory" );
}

static inline void cpu_fxrstor(void *ctx) {
    asm volatile ( "fxrstor [%0]" : : "r" (ctx) : "memory" );
}

static inline int cpu_toggleint(int state) {
    int ret = cpu_intstate();
    if(state) cpu_enableints();
    else cpu_disableints();
    return ret;
}

#define CPUID_XSAVE (uint32_t)(1 << 26)
#define CPUID_AVX (uint32_t)(1 << 28)
#define CPUID_AVX512 (uint32_t)(1 << 16)

extern uint64_t fpu_storesize;
// for changing between xsave and (legacy) fxsave
extern void (*fpu_save)(void *);
extern void (*fpu_restore)(void *);

static inline int cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    uint32_t cpuidmax = 0;
    asm volatile (
        "cpuid"
        : "=a" (cpuidmax)
        : "a" (leaf & 0x80000000)
        : "rbx", "rcx", "rdx"
    );
    if(leaf > cpuidmax) return 0;
    asm volatile (
        "cpuid"
        : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
        : "a" (leaf), "c" (subleaf)
    );
    return 1;
}

#endif
