#ifndef __CPU__SMP_H__
#define __CPU__SMP_H__

#include <limine.h>

extern uint64_t cpucount; 

void smp_initcpu(struct limine_smp_info *info);
void smp_init(struct limine_smp_response *smp);

#endif
