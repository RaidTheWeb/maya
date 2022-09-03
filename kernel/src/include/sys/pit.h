#ifndef __SYS__PIT_H__
#define __SYS__PIT_H__

#include <stdint.h>

#define PIT_DIVEND 1193180

uint16_t pit_curcount(void);
void pit_setreload(uint16_t count);
void pit_setfreq(uint64_t freq);
void pit_init(void);

#endif
