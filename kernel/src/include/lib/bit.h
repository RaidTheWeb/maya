#ifndef __LIB__BIT_H__
#define __LIB__BIT_H__

#include <stdint.h>

#define bittest(_bitmap, _index) ({ \
        uint64_t *fbitmap = (uint64_t *)_bitmap; \
        int bitstype = sizeof(uint64_t) * 8; \
        uint64_t ti = _index % bitstype; \
        uint64_t ts = fbitmap[_index / bitstype]; \
        ((ts >> ti) & 1) != 0; \
    })

#define bitset(_bitmap, _index) ({ \
        uint64_t *fbitmap = (uint64_t *)_bitmap; \
        int bitstype = sizeof(uint64_t) * 8; \
        uint64_t ti = _index % bitstype; \
        fbitmap[_index / bitstype] |= 1 << ti; \
    })

#define bitreset(_bitmap, _index) ({ \
        uint64_t *fbitmap = (uint64_t *)_bitmap; \
        int bitstype = sizeof(uint64_t) * 8; \
        uint64_t ti = _index % bitstype; \
        fbitmap[_index / bitstype] &= ~(1 << ti); \
    })

#endif
