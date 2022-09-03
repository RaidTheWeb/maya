#include <mm/mm.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <stddef.h>
#include <cpu/cpu.h>
#include <lib/align.h>
#include <lib/assert.h>
#include <lib/stdio.h>

pagemap_t *kernelpagemap = NULL;
uint8_t vmm_initialised = 0;

pagemap_t *vmm_initpagemap(void) {
    uint64_t *top = (uint64_t *)pmm_alloc(1);
    if(top == NULL) {
        printf("[vmm]: Failed to allocate pagemap\n");
        for(;;) asm("hlt");
    }

    uint64_t *p1 = (uint64_t *)((uint64_t)top + HIGHER_HALF);
    uint64_t *p2 = (uint64_t *)((uint64_t)kernelpagemap->top + HIGHER_HALF);
    for(uint64_t i = 256; i < 512; i++) p1[i] = p2[i]; // copy kernel pagemap to new pagemap
    pagemap_t *pagemap = malloc(sizeof(pagemap_t));
    pagemap->top = top;
    pagemap->ranges = (typeof(pagemap->ranges))LIST_INIT;
    return pagemap;
}

uint64_t *vmm_virtpte(pagemap_t *pagemap, uintptr_t virt, int allocate) {
    uint64_t pml4_entry = (virt & ((uint64_t)0x1ff << 39)) >> 39;
    uint64_t pml3_entry = (virt & ((uint64_t)0x1ff << 30)) >> 30;
    uint64_t pml2_entry = (virt & ((uint64_t)0x1ff << 21)) >> 21;
    uint64_t pml1_entry = (virt & ((uint64_t)0x1ff << 12)) >> 12;

    uint64_t *pml4 = pagemap->top;
    uint64_t *pml3 = vmm_getnextlevel(pml4, pml4_entry, allocate);
    if(pml3 == NULL) return NULL;
    uint64_t *pml2 = vmm_getnextlevel(pml3, pml3_entry, allocate);
    if(pml2 == NULL) return NULL;
    uint64_t *pml1 = vmm_getnextlevel(pml2, pml2_entry, allocate);
    if(pml1 == NULL) return NULL;

    return (uint64_t *)((uint64_t)&pml1[pml1_entry] + HIGHER_HALF);

}

uintptr_t vmm_virtphys(pagemap_t *pagemap, uintptr_t virt) {
    uint64_t *pte = vmm_virtpte(pagemap, virt, 0);
    if(pte == NULL) return (uintptr_t)-1;
    if((pte[0] & 1) != 0) return (uintptr_t)-1;
    return pte[0] & ~((uint64_t)0xfff);

}

void vmm_switchto(pagemap_t *pagemap) {
    uint64_t *top = pagemap->top;

    asm volatile (
        "mov cr3, %0"
        : : "r" (top)
        : "memory"
    );
}

uint64_t *vmm_getnextlevel(uint64_t *level, uint64_t idx, int allocate) {
    uint64_t *ret = 0;

    uint64_t *entry = (uint64_t *)((uint64_t)level + HIGHER_HALF + idx * 8);

    if((entry[0] & 0x01) != 0) ret = (uint64_t *)(entry[0] & ~((uint64_t)0xfff));
    else {
        if(!allocate) return NULL;

        ret = pmm_alloc(1);
        if(ret == NULL) return NULL;
        entry[0] = (uint64_t)ret | 0x07;
    }

    return ret;
}

int vmm_mappage(pagemap_t *pagemap, uintptr_t virt, uintptr_t phys, uint64_t flags) {
    spinlock_acquire(&pagemap->lock);

    uint64_t pml4_entry = (virt & ((uint64_t)0x1ff << 39)) >> 39;
    uint64_t pml3_entry = (virt & ((uint64_t)0x1ff << 30)) >> 30;
    uint64_t pml2_entry = (virt & ((uint64_t)0x1ff << 21)) >> 21;
    uint64_t pml1_entry = (virt & ((uint64_t)0x1ff << 12)) >> 12;

    uint64_t *pml4 = pagemap->top;
    uint64_t *pml3 = vmm_getnextlevel(pml4, pml4_entry, 1);
    if(pml3 == NULL) return 0;
    uint64_t *pml2 = vmm_getnextlevel(pml3, pml3_entry, 1);
    if(pml2 == NULL) return 0;
    uint64_t *pml1 = vmm_getnextlevel(pml2, pml2_entry, 1);
    if(pml1 == NULL) return 0;

    uint64_t *entry = (uint64_t *)((uint64_t)pml1 + HIGHER_HALF + pml1_entry * 8);
    entry[0] = phys | flags; // map the page with flags

    spinlock_release(&pagemap->lock);
    return 1;
}

int vmm_flagpage(pagemap_t *pagemap, uintptr_t virt, uint64_t flags) {
    uint64_t *pte = vmm_virtpte(pagemap, virt, 0);
    if(pte == NULL) return 0;

    *pte &= ~((uint64_t)0xfff);
    *pte |= flags;

    uint64_t curcr3 = cpu_readcr3();
    if(curcr3 == (uint64_t)pagemap->top) cpu_invlpg(virt);
    return 1;
}

int vmm_unmappage(pagemap_t *pagemap, uintptr_t virt) {
    uint64_t *pte = vmm_virtpte(pagemap, virt, 0);
    if(pte == NULL) return 0;

    *pte = 0; // reset page table entry

    uint64_t curcr3 = cpu_readcr3();
    if(curcr3 == (uint64_t)pagemap->top) cpu_invlpg(virt);
    return 1;
}

extern char kernelendaddr[];

void vmm_init(struct limine_memmap_response *memmap, struct limine_kernel_address_response *base) {
    printf("[vmm]: Kernel physical base 0x%016llx\n", base->physical_base);
    printf("[vmm]: Kernel virtual base 0x%016llx\n", base->virtual_base);

    kernelpagemap = malloc(sizeof(pagemap_t)); // allocate pagemap
    if(kernelpagemap == NULL) {
        printf("[vmm]: Failed to allocate kernel pagemap\n");
        for(;;) asm("hlt");
    }

    kernelpagemap->top = pmm_alloc(1);
    if(kernelpagemap->top == NULL) {
        printf("[vmm]: Failed to allocate kernel pagemap top\n");
        for(;;) asm("hlt");
    }

    for(uint64_t i = 256; i < 512; i++) {
        assert(vmm_getnextlevel(kernelpagemap->top, i, 1));
    }

    uintptr_t virt = base->virtual_base;
    uintptr_t phys = base->physical_base;
    uint64_t len = alignup((uintptr_t)kernelendaddr, PAGE_SIZE) - virt;

    printf("[vmm]: Mapping PMRS 0x%016llx->0x%016llx (+0x%x)\n", phys, virt, len);

    for(uint64_t i = 0; i < len; i += PAGE_SIZE) {
        assert(vmm_mappage(kernelpagemap, virt + i, phys + i, 0x03));
    }

    // 4GiB
    for(uint64_t i = PAGE_SIZE; i < 0x100000000; i += PAGE_SIZE) {
        assert(vmm_mappage(kernelpagemap, i, i, 0x03));
        assert(vmm_mappage(kernelpagemap, i + HIGHER_HALF, i, 0x03));
    }

    struct limine_memmap_entry **entries = memmap->entries;
    for(uint64_t i = 0; i < memmap->entry_count; i++) {
        uint64_t base = aligndown(entries[i]->base, PAGE_SIZE);
        uint64_t top = alignup(entries[i]->base + entries[i]->length, PAGE_SIZE);
        if(top <= 0x100000000) continue;
        for(uint64_t j = base; j < top; j += PAGE_SIZE) {
            if(j < 0x100000000) continue;
            assert(vmm_mappage(kernelpagemap, j, j, 0x03));
            assert(vmm_mappage(kernelpagemap, j + HIGHER_HALF, j, 0x03));
        }
    }

    vmm_switchto(kernelpagemap);
    vmm_initialised = 1;
    printf("[vmm]: VMM initialised\n");
}


