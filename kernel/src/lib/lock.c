#include <lib/lock.h>

int spinlock_testacq(spinlock_t *lock) {
    int ret;
    asm volatile (
        "lock btr %0, 0;"
        : "+m" (lock->lock), "=@ccc" (ret)
        : : "memory"
    );
    return ret;
}

void spinlock_acquire(spinlock_t *lock) {
    for(uint64_t i = 0; i < DEADLOCKMAX; i++) {
        if(spinlock_testacq(lock)) break;
        asm volatile("pause");
    }
}
