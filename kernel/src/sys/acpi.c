#include <sys/acpi.h>
#include <mm/mm.h>
#include <stddef.h>
#include <lib/stdio.h>
#include <lib/string.h>
#include <sys/madt.h>

rsdp_t *acpi_rsdp = NULL;
rsdt_t *acpi_rsdt = NULL;

int usexsdt(void) {
    return acpi_rsdp->rev >= 2 && acpi_rsdp->xsdtaddr != 0;
}

void acpi_init(struct limine_rsdp_response *rsdp) {
    acpi_rsdp = rsdp->address;

    if(acpi_rsdp == NULL) {
        printf("[acpi]: ACPI Not supported\n");
        for(;;) asm("hlt");
    }

    if(usexsdt()) acpi_rsdt = (rsdt_t *)(acpi_rsdp->xsdtaddr + HIGHER_HALF);
    else acpi_rsdt = (rsdt_t *)((uint64_t)acpi_rsdp->rsdtaddr + HIGHER_HALF);

    printf("[acpi]: Revision %d\n", acpi_rsdp->rev);
    if(usexsdt()) printf("[acpi]: Using XSDT\n");
    printf("[acpi]: R/XSDT found at 0x%016llx\n", acpi_rsdt);

    sdt_t *fadt = acpi_findsdt("FACP", 0);
    if(fadt != NULL && fadt->len >= 116) {
        uint32_t flags = *((uint32_t *)fadt + 28);

        if((flags & (1 << 20))) {
            printf("[acpi]: HW Reduced ACPI not supported\n");
            for(;;) asm("hlt");
        }
    }

    madt_init();
}

void *acpi_findsdt(char sign[4], uint64_t idx) {
    uint64_t entry = (acpi_rsdt->header.len - sizeof(sdt_t)) / (usexsdt() ? 8 : 4);

    for(uint64_t i = 0; i < entry; i++) {
        sdt_t *sdt = NULL;

        if(usexsdt()) sdt = (sdt_t *)(*((uint64_t *)acpi_rsdt->data + i) + HIGHER_HALF); 
        else sdt = (sdt_t *)(*((uint32_t *)acpi_rsdt->data + i) + HIGHER_HALF);

        if(memcmp(sdt->sign, sign, 4) != 0) continue;

        if(idx > 0) {
            idx--;
            continue;
        }
        printf("[acpi]: Found signature %s at 0x%016llx\n", sign, sdt);
        return sdt;
    }

    printf("[acpi]: Signature %s not found\n", sign);
    return NULL;
}
