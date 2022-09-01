#ifndef __LIB__STRING_H__
#define __LIB__STRING_H__

#include <stdint.h>

void *memcpy(void *dest, void *src, uint64_t size);
void *memset(void *dest, int c, uint64_t size);
int memcmp(void *s1, void *s2, uint64_t size);

#endif
