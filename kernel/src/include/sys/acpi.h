#ifndef __SYS__ACPI_H__
#define __SYS__ACPI_H__

#include <stdint.h>
#include <limine.h>

typedef struct __attribute__((packed)) {
    char sign[4];
    uint32_t len;
    uint8_t rev;
    uint8_t checksum;
    char oemid[6];
    char oemtableid[8];
    uint32_t oemrev;
    uint32_t creatorid;
    uint32_t creatorrev;
} sdt_t;

typedef struct __attribute__((packed)) {
    char sign[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t rev;
    uint32_t rsdtaddr;
    uint32_t len;
    uint64_t xsdtaddr;
    uint8_t extchecksum;
    char rsvd[3];
} rsdp_t;

typedef struct __attribute__((packed)) {
    sdt_t header;
    char data[];
} rsdt_t;

extern rsdp_t *acpi_rsdp;
extern rsdt_t *acpi_rsdt;

int usexsdt(void);
void acpi_init(struct limine_rsdp_response *rsdp);
void *acpi_findsdt(char sign[4], uint64_t idx);

#endif
