#ifndef __MM__VMM_H__
#define __MM__VMM_H__

#include <stdint.h>
#include <limine.h>
#include <lib/lock.h>
#include <lib/list.h>
#include <cpu/cpu.h>

#define PTE_PRESENT ((uint64_t)1 << (uint64_t)0)
#define PTE_WRITABLE ((uint64_t)1 << (uint64_t)1)
#define PTE_USER ((uint64_t)1 << (uint64_t)2)
#define PTE_NX ((uint64_t)1 << (uint64_t)3)

#define PTE_MASK 0x000ffffffffff000
#define PTE_ADDR(val) ((val) & PTE_MASK)
#define PTE_FLAGS(val) ((val) & ~PTE_MASK)

#define PAGE_SIZE 0x1000

typedef struct {
    spinlock_t lock;
    uint64_t *top;
    LIST_TYPE(void *) ranges;
} pagemap_t;

extern uint8_t vmm_initialised;
extern pagemap_t *kernelpagemap;

#define PROT_NONE 0x00
#define PROT_READ 0x01
#define PROT_WRITE 0x02
#define PROT_EXEC 0x04
#define MAP_PRIV 0x01
#define MAP_SHARE 0x02
#define MAP_FIXED 0x04
#define MAP_ANON 0x08

typedef struct __mmaprangelocal mmaprangelocal_t;
typedef struct __mmaprangeglobal mmaprangeglobal_t;

struct __mmaprangeglobal {
    pagemap_t *shadow;
    LIST_TYPE(mmaprangelocal_t *) locals;
    uintptr_t base;
    uint64_t len;
    int64_t off;
};

struct __mmaprangelocal {
    pagemap_t *pagemap;
    mmaprangeglobal_t *global;
    uintptr_t base;
    uint64_t len;
    int64_t off;
    int prot;
    int flags;
};

pagemap_t *vmm_initpagemap(void);
uint64_t *vmm_virtpte(pagemap_t *pagemap, uintptr_t virt, int allocate);
uintptr_t vmm_virtphys(pagemap_t *pagemap, uintptr_t virt);
void vmm_switchto(pagemap_t *pagemap);
uint64_t *vmm_getnextlevel(uint64_t *level, uint64_t idx, int allocate);
int vmm_mappage(pagemap_t *pagemap, uintptr_t virt, uintptr_t phys, uint64_t flags);
int vmm_flagpage(pagemap_t *pagemap, uintptr_t virt, uint64_t flags);
int vmm_unmappage(pagemap_t *pagemap, uintptr_t virt);

void vmm_init(struct limine_memmap_response *memmap, struct limine_kernel_address_response *base);

#endif
