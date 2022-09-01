#ifndef __MM__PMM_H__
#define __MM__PMM_H__

#include <limine.h>
#include <stdint.h>
#include <stddef.h>

void pmm_init(struct limine_memmap_response *memmap);
void *pmm_alloc(uint64_t size);
void *pmm_allocnz(uint64_t size);
void *pmm_realloc(void *ptr, uint64_t size);
void pmm_free(void *ptr, uint64_t size);

void *malloc(uint64_t size);
void *realloc(void *ptr, uint64_t size);
void free(void *ptr);

void *pmm_slaballoc(uint64_t size);
void *pmm_slabrealloc(void *ptr, uint64_t size);
void pmm_slabfree(void *ptr);

uint64_t pmm_totalpages(void);
uint64_t pmm_usedpages(void);
uint64_t pmm_freepages(void);
uint64_t pmm_rsvdpages(void);

#endif
