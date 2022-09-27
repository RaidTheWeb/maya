#ifndef __SYS__MADT_H__
#define __SYS__MADT_H__

#include <stdint.h>
#include <sys/acpi.h>
#include <lib/list.h>

typedef struct __attribute__((packed)) {
    sdt_t header;
    uint32_t localconaddr;
    uint32_t flags;
    char data[];
} madt_t;

typedef struct __attribute__((packed)) {
    uint8_t id;
    uint8_t len;
} madtheader_t;

typedef struct __attribute__((packed)) {
    madtheader_t header;
    uint8_t procid;
    uint8_t apicid;
    uint32_t flags;
} madtlapic_t;

typedef struct __attribute__((packed)) {
    madtheader_t header;
    uint8_t apicid;
    uint8_t rsvd;
    uint32_t addr;
    uint32_t gsib;
} madtioapic_t;

typedef struct __attribute__((packed)) {
    madtheader_t header;
    uint8_t bussrc;
    uint8_t irqsrc;
    uint32_t gsi;
    uint16_t flags;
} madtiso_t;

typedef struct __attribute__((packed)) {
    madtheader_t header;
    uint8_t proc;
    uint16_t flags;
    uint8_t lint;
} madtnmi_t;

extern LIST_TYPE(madtlapic_t *) madt_lapics;
extern LIST_TYPE(madtioapic_t *) madt_ioapics;
extern LIST_TYPE(madtiso_t *) madt_isos;
extern LIST_TYPE(madtnmi_t *) madt_nmis;

void madt_init(void);

#endif
