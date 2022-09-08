#ifndef __SCHED__SCHED_H__
#define __SCHED__SCHED_H__

#include <stdint.h>
#include <stddef.h>
#include <cpu/cpu.h>
#include <lib/list.h>
#include <lib/lock.h>

#define MAXQUEUEDTHREADS 65536

typedef struct __cthread cthread_t;

struct __cthread {
    int curcpu;
    uint64_t kstack;
    uint64_t ustack;
    uint64_t tid;
    int inqueue;
    spinlock_t lock;
    cpustate_t state;
    uint64_t timeslice;
    spinlock_t yieldawait;
    uint64_t cr3; // pagemap location
    LIST_TYPE(void *)stacks;
};

extern LIST_TYPE(cthread_t *) threadtable;

extern uint8_t sched_vec;

void sched_init(void);
cthread_t *sched_newkthread(void *func, void *arg, int enqueue);
void sched_await(void);
int sched_enqueue(cthread_t *thread);
int sched_dequeue(cthread_t *thread);

#endif
