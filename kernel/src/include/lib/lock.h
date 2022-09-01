#ifndef __LIB__LOCK_H__
#define __LIB__LOCK_H__

#include <stdint.h>

typedef struct {
    int lock;
} spinlock_t;

#define LOCKINIT ((spinlock_t){ .lock = 1 })

#define DEADLOCKMAX 5000000

int spinlock_testacq(spinlock_t *lock);
void spinlock_acquire(spinlock_t *lock);

__attribute__((always_inline)) static inline void spinlock_release(spinlock_t *lock) {
    asm volatile (
        "lock bts %0, 0;"
        : "+m" (lock->lock)
        : : "memory", "cc"
    );
}

#endif
