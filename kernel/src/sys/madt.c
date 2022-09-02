#include <sys/madt.h>
#include <sys/acpi.h>
#include <lib/stdio.h>

typeof(madt_lapics) madt_lapics = LIST_INIT;
typeof(madt_ioapics) madt_ioapics = LIST_INIT;
typeof(madt_isos) madt_isos = LIST_INIT;
typeof(madt_nmis) madt_nmis = LIST_INIT;

void madt_init(void) {
    madt_t *madt = (madt_t *)acpi_findsdt("APIC", 0);
    if(madt == NULL) {
        printf("[madt]: System does not support MADT\n");
        for(;;) asm("hlt");
    }

    uint64_t off = 0;

    while(1) {
        if(off + (sizeof(madt_t) - 1) >= madt->header.len) break;

        madtheader_t *header = (madtheader_t *)(&madt->begin + off);
        switch(header->id) {
            case 0:
                printf("[madt]: Found Local APIC #%d\n", madt_lapics.length);
                LIST_PUSHBACK(&madt_lapics, (madtlapic_t *)header);
                break;
            case 1:
                printf("[madt]: Found IO APIC #%d\n", madt_ioapics.length);
                LIST_PUSHBACK(&madt_ioapics, (madtioapic_t *)header);
                break;
            case 2:
                printf("[madt]: Found ISO #%d\n", madt_isos.length);
                LIST_PUSHBACK(&madt_isos, (madtiso_t *)header);
                break;
            case 4:
                printf("[madt]: Found NMI #%d\n", madt_nmis.length);
                LIST_PUSHBACK(&madt_nmis, (madtnmi_t *)header);
                break;
        }

        off += header->len;
    }
}
