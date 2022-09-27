#include <lib/string.h>

void *memset(void *dest, int c, uint64_t size) {
    for(uint64_t i = 0; i < size; i++) ((uint8_t *)dest)[i] = (uint8_t)c;
    return dest;
}

void *memcpy(void *dest, void *src, uint64_t size) {
    for(uint64_t i = 0; i < size; i++) ((uint8_t *)dest)[i] = ((uint8_t *)src)[i];
    return dest;
}

int memcmp(void *s1, void *s2, uint64_t size) {
    for(uint64_t i = 0; i < size; i++)
        if(((uint8_t *)s1)[i] != ((uint8_t *)s2)[i]) return ((uint8_t *)s1)[i] < ((uint8_t *)s2)[i] ? -1 : 1; // different (with comparison)

    return 0; // equal
}
