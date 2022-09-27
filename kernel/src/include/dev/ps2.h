#ifndef __DEV__PS2_H__
#define __DEV__PS2_H__

#include <stdint.h>

uint8_t ps2_read(void);
void ps2_write(uint16_t port, uint8_t val);
uint8_t ps2_readcfg(void);
void ps2_writecfg(uint8_t cfg);
void ps2_init(void);

#endif
