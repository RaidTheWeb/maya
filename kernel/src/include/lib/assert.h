#ifndef __LIB__ASSERT_H__
#define __LIB__ASSERT_H__

#define assert(check) ({ \
    if(!(check)) { \
        printf("Assertion failed in %s (%s:%d)\n", __func__, __FILE__, __LINE__); \
        for(;;) asm("hlt"); \
    } \
})

#endif
