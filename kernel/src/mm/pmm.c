#include <mm/pmm.h>
#include <lib/lock.h>
#include <lib/align.h>
#include <mm/mm.h>
#include <lib/string.h>
#include <lib/stdio.h>
#include <lib/bit.h>

typedef struct {
    spinlock_t lock;
    void **first;
    uint64_t size;
} slab_t;

typedef struct {
    slab_t *slab; // points to a specific slab (not utilising a direct slab so that this can fit in less space)
} slabheader_t;

// keeps track of the number of pages the allocation takes up, along with the size of the allocation
typedef struct {
    uint64_t pages;
    uint64_t size;
} allocmetadata_t;

static slab_t slabs[10]; // allocate a few slabs into .data

// find a slab we can use
static slab_t *pmm_iterslabs(uint64_t size) {
    for(uint64_t i = 0; i < sizeof(slabs) / sizeof(slab_t); i++) {
        slab_t *slab = &slabs[i];
        if(slab->size >= size) return slab;
    }
    return NULL; // we couldn't find a suitable slab
}

static void pmm_initslab(slab_t *slab, uint64_t size) {
    slab->lock = LOCKINIT;
    slab->first = pmm_allocnz(1) + HIGHER_HALF;
    slab->size = size;

    uint64_t off = alignup(sizeof(slabheader_t), size);
    uint64_t av = 0x1000 - off;

    slabheader_t *ptr = (slabheader_t *)slab->first; // grab the header for the slab
    ptr->slab = slab;
    slab->first = (void **)((void *)slab->first + off);

    void **arr = (void **)slab->first;
    uint64_t max = av / size - 1;
    uint64_t fac = size / sizeof(void *);

    // shift all the data
    for(uint64_t i = 0; i < max; i++) arr[i * fac] = &arr[(i + 1) * fac];
    arr[max * fac] = NULL;
}

static void *pmm_allocfrom(slab_t *slab) {
    spinlock_acquire(&slab->lock);
    if(slab->first == NULL) pmm_initslab(slab, slab->size);
    void **old = slab->first;
    slab->first = *old;
    memset(old, 0, slab->size);

    spinlock_release(&slab->lock);
    return old;
}

static void pmm_freein(slab_t *slab, void *ptr) {
    spinlock_acquire(&slab->lock);

    if(ptr != NULL) {
        void **new = ptr;
        *new = slab->first;
        slab->first = new;
    }

    spinlock_release(&slab->lock);
}

void *pmm_slaballoc(uint64_t size) {
    slab_t *slab = pmm_iterslabs(size); // find a suitable location on the heap
    if(slab != NULL) return pmm_allocfrom(slab);

    uint64_t count = divroundup(size, 0x1000);
    void *ret = pmm_alloc(count + 1);
    if(ret == NULL) return NULL;

    ret += HIGHER_HALF;
    allocmetadata_t *meta = (allocmetadata_t *)ret; // turn this into allocation metadata

    meta->pages = count;
    meta->size = size;

    return ret + 0x1000; // gives us the next page slab
}

void *pmm_slabrealloc(void *ptr, uint64_t size) {
    if(ptr == NULL) return pmm_slaballoc(size);

    if(!((uintptr_t)ptr & 0x7fff)) {
        allocmetadata_t *meta = (allocmetadata_t *)(ptr - 0x1000);
        if(divroundup(meta->size, 0x1000) == divroundup(size, 0x1000)) {
            meta->size = size;
            return ptr;
        }

        void *newptr = pmm_slaballoc(size);
        if(newptr == NULL) return NULL;

        if(meta->size > size) memcpy(newptr, ptr, size);
        else memcpy(newptr, ptr, meta->size);

        pmm_slabfree(ptr);
        return newptr;
    }

    slab_t *slab = ((slabheader_t *)((uintptr_t)ptr & ~0xfff))->slab; // grab the old pointer to the slab

    if(size > slab->size) {
        void *newptr = pmm_slaballoc(size);
        if(newptr == NULL) return NULL;

        memcpy(newptr, ptr, slab->size); // copy data to new pointer
        pmm_freein(slab, ptr);
        return newptr;
    }

    return ptr;
}

void pmm_slabfree(void *ptr) {
    if(ptr == NULL) return;

    if(!((uintptr_t)ptr & 0xfff)) {
        allocmetadata_t *meta = (allocmetadata_t *)(ptr - 0x1000);
        pmm_free((void *)meta - HIGHER_HALF, meta->pages + 1);
        return;
    }

    pmm_freein(((slabheader_t *)((uintptr_t)ptr & ~0xfff))->slab, ptr);
}

void *malloc(uint64_t size) {
    void *res = pmm_slaballoc(size);

    if(res == NULL) {
        printf("[malloc]: Kernel out of memory\n");
        return NULL;
    }

    return res;
}

void *realloc(void *ptr, uint64_t size) {
    void *res = pmm_slabrealloc(ptr, size);
    if(res == NULL) {
        printf("[realloc]: Kernel out of memory\n");
        return NULL;
    }

    return res;
}

void free(void *ptr) {
    if(ptr == NULL) return;

    pmm_slabfree(ptr);
}

static spinlock_t pmm_lock;
static uint8_t *pmm_bitmap = NULL;
static uint64_t pmm_highest = 0;
static uint64_t pmm_lastused = 0;
static uint64_t pmm_usable = 0;
static uint64_t pmm_used = 0;
static uint64_t pmm_reserved = 0;

void pmm_init(struct limine_memmap_response *memmap) {
    uint64_t highestaddr = 0;

    for(uint64_t i = 0; i < memmap->entry_count; i++) {
        printf("[pmm]: Memory map entry (%d) 0x%llx->0x%llx+0x%llx, (%s)\n", i, memmap->entries[i]->base, memmap->entries[i]->base + memmap->entries[i]->length, memmap->entries[i]->length, memmap->entries[i]->type == LIMINE_MEMMAP_USABLE ? "USABLE" : memmap->entries[i]->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE ? "BOOTLOADER RECLAIMABLE" : memmap->entries[i]->type == LIMINE_MEMMAP_RESERVED ? "RESERVED" : "OTHER");

        switch(memmap->entries[i]->type) {
            case LIMINE_MEMMAP_USABLE:
                pmm_usable += divroundup(memmap->entries[i]->length, 0x1000);
                highestaddr = max(highestaddr, memmap->entries[i]->base + memmap->entries[i]->length);
                break;
            case LIMINE_MEMMAP_RESERVED:
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
            case LIMINE_MEMMAP_ACPI_NVS:
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
            case LIMINE_MEMMAP_KERNEL_AND_MODULES:
                pmm_reserved += divroundup(memmap->entries[i]->length, 0x1000);
                break;
        }
    }

    pmm_highest = highestaddr / 0x1000;
    uint64_t bitmapsize = alignup(pmm_highest / 8, 0x1000);

    printf("[pmm]: Found the highest address as 0x%016llx\n", highestaddr);
    printf("[pmm]: Bitmap size required for mapping: %luKiB\n", bitmapsize / 1024);

    for(uint64_t i = 0; i < memmap->entry_count; i++) {
        if(memmap->entries[i]->type != LIMINE_MEMMAP_USABLE) continue; // skip over every instance
        if(memmap->entries[i]->length >= bitmapsize) {
            pmm_bitmap = (uint8_t *)(memmap->entries[i]->base + HIGHER_HALF);
            memset(pmm_bitmap, 0xff, bitmapsize);
            memmap->entries[i]->length -= bitmapsize;
            memmap->entries[i]->base += bitmapsize;

            break;
        }
    }

    printf("[pmm]: Bitmap allocated to 0x%016llx\n", (uintptr_t)pmm_bitmap);

    for(uint64_t i = 0; i < memmap->entry_count; i++) {
        if(memmap->entries[i]->type != LIMINE_MEMMAP_USABLE) continue;
        for(uint64_t j = 0; j < memmap->entries[i]->length; j += 0x1000) bitreset(pmm_bitmap, (memmap->entries[i]->base + j) / 0x1000);
    }

    printf("[pmm]: Available memory: %luMiB\n", (pmm_usable * 0x1000) / 1024 / 1024);
    printf("[pmm]: Reserved memory: %luMiB\n", (pmm_reserved * 0x1000) / 1024 / 1024);

    // initialise all memory slabs
    pmm_initslab(&slabs[0], 8);
    pmm_initslab(&slabs[1], 16);
    pmm_initslab(&slabs[2], 24);
    pmm_initslab(&slabs[3], 32);
    pmm_initslab(&slabs[4], 48);
    pmm_initslab(&slabs[5], 64);
    pmm_initslab(&slabs[6], 128);
    pmm_initslab(&slabs[7], 256);
    pmm_initslab(&slabs[8], 512);
    pmm_initslab(&slabs[9], 1024);
}

static void *pmm_inner(uint64_t size, uint64_t limit) {
    uint64_t p = 0;

    while(pmm_lastused < limit) {
        if(!bittest(pmm_bitmap, pmm_lastused++)) {
            p++;
            if(p == size) {
                uint64_t addr = pmm_lastused - size;
                for(uint64_t i = 0; i < pmm_lastused; i++) bitset(pmm_bitmap, i);
                return (void *)(addr * 0x1000);
            }
        } else p = 0;
    }

    return NULL;
}

void *pmm_allocnz(uint64_t size) {
    spinlock_acquire(&pmm_lock);

    uint64_t last = pmm_lastused;
    void *ret = pmm_inner(size, pmm_highest);

    if(ret == NULL) {
        pmm_lastused = 0;
        ret = pmm_inner(size, last); // secondary allocation (if the first fails)
    }

    if(ret == NULL) {
        printf("[pmm]: Secondary allocation failed\n");
        return NULL;
    }
    pmm_used += size; // increase the number of pages we've currently got in use

    spinlock_release(&pmm_lock);
    return ret;
}

void *pmm_alloc(uint64_t size) {
    void *ret = pmm_allocnz(size);
    if(ret != NULL) memset(ret + HIGHER_HALF, 0, size * 0x1000); // clear memory page
    return ret;
}

void pmm_free(void *ptr, uint64_t size) {
    spinlock_acquire(&pmm_lock);
    uint64_t page = (uint64_t)ptr / 0x1000;
    for(uint64_t i = 0; i < page + size; i++) bitreset(pmm_bitmap, i);
    pmm_used -= size; // reduce the number of pages we have recorded (as we've just freed one)
    spinlock_release(&pmm_lock);
}

uint64_t pmm_totalpages(void) {
    return pmm_usable;
}

uint64_t pmm_usedpages(void) {
    return pmm_used;
}

uint64_t pmm_freepages(void) {
    return pmm_usable - pmm_used;
}

uint64_t pmm_rsvdpages(void) {
    return pmm_reserved;
}
