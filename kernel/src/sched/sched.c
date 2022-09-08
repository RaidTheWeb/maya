#include <sched/sched.h>
#include <lib/stdio.h>
#include <sys/apic.h>
#include <sys/idt.h>
#include <mm/mm.h>
#include <mm/pmm.h>
#include <mm/vmm.h>

static cthread_t *sched_queue[MAXQUEUEDTHREADS];
static uint64_t sched_queueptr = 0;

uint8_t sched_vec = 0;

static spinlock_t sched_lock = LOCKINIT;

typeof(threadtable) threadtable = LIST_INIT;

static int sched_nextthread(int prev) {
    spinlock_acquire(&sched_lock);

    int cpunum = cpu_current()->cpunum;

    if(prev >= (int)sched_queueptr) prev = 0;

    int i = prev + 1;

    while(1) {
        if(i >= (int)sched_queueptr) i = 0;

        cthread_t *thread = sched_queue[i];

        if(thread != NULL) {
            // printf("found a thread\n");
            if(thread->curcpu == cpunum || spinlock_testacq(&thread->lock)) {
                spinlock_release(&sched_lock);
                return i;
            }
        }

        if(i == prev) break;

        i++;
    }

    spinlock_release(&sched_lock);
    return -1;
}

static void sched_entry(uint32_t vec, cpustate_t *state) {
    (void)vec;

    cpulocal_t *local = cpu_current();

    local->active = 1;
    
    cthread_t *curthread = cpu_current()->curthread;

    int new = sched_nextthread(local->lastrunqueue);

    if(curthread != NULL) {
        spinlock_release(&curthread->yieldawait);

        if(new == local->lastrunqueue) {
            lapic_eoi();
            lapic_timeroneshot(local, sched_vec, curthread->timeslice);
            return;
        }

        curthread->state = *state; // update state

        curthread->cr3 = cpu_readcr3();

        curthread->curcpu = -1;
        spinlock_release(&curthread->lock);
    }

    if(new == -1) {
        local->curthread = NULL;
        local->lastrunqueue = 0;
        local->active = 0;
        vmm_switchto(kernelpagemap);
        sched_await();
    }

    curthread = sched_queue[new];
    local->lastrunqueue = new;

    local->curthread = curthread;

    if(cpu_readcr3() != curthread->cr3) cpu_writecr3(curthread->cr3);

    curthread->curcpu = local->cpunum;

    lapic_eoi();
    lapic_timeroneshot(local, sched_vec, curthread->timeslice);

    cpustate_t *newstate = &curthread->state;

    // printf("spinning up\n");
    asm volatile (
        "mov rsp, %0\n"
        "pop rax\n"
        "mov ds, eax\n"
        "pop rax\n"
        "mov es, eax\n"
        "pop rax\n"
        "pop rbx\n"
        "pop rcx\n"
        "pop rdx\n"
        "pop rsi\n"
        "pop rdi\n"
        "pop rbp\n"
        "pop r8\n"
        "pop r9\n"
        "pop r10\n"
        "pop r11\n"
        "pop r12\n"
        "pop r13\n"
        "pop r14\n"
        "pop r15\n"
        "add rsp, 8\n"
        "iretq\n"
        : : "rm" (newstate)
        : "memory"
    );
}

void sched_init(void) {
    sched_vec = idt_allocvec();

    // setup scheduler information
    isr[sched_vec] = sched_entry;
    idt_updateist(sched_vec, 1);

    printf("[sched]: Scheduler initialised\n");
}

void sched_await(void) {
    cpu_toggleint(0);
    printf("scheduler current CPU #%d\n", cpu_current()->cpunum); 
    lapic_timeroneshot(cpu_current(), sched_vec, 20000); // schedule for future
    cpu_toggleint(1);
    printf("halting\n");
    for(;;) asm("hlt"); // halt while still allowing our attention to be diverted to scheduling
}

int sched_enqueue(cthread_t *thread) {
    if(thread->inqueue) return 1;

    spinlock_acquire(&sched_lock);

    for(uint64_t i = 0; i < MAXQUEUEDTHREADS; i++) {
        if(sched_queue[i] == NULL) {
            sched_queue[i] = thread;
            thread->inqueue = 1;

            if(i >= sched_queueptr) sched_queueptr = i + 1;

            spinlock_release(&sched_lock);
            return 1;
        }
    }

    spinlock_release(&sched_lock);
    return 0;
}

int sched_dequeue(cthread_t *thread) {
    if(!thread->inqueue) return 1;

    spinlock_acquire(&sched_lock);

    for(uint64_t i = 0; i < sched_queueptr; i++) {
        if(sched_queue[i] == thread) {
            sched_queue[i] = NULL;
            thread->inqueue = 0;

            if(i == sched_queueptr - 1) sched_queueptr--;

            spinlock_release(&sched_lock);
            return 1;
        }
    }

    spinlock_release(&sched_lock);
    return 0;
}

#define KTHREADSTACKSIZE 0x40000

cthread_t *sched_newkthread(void *func, void *arg, int enqueue) {
    cthread_t *thread = malloc(sizeof(cthread_t));
    LIST_PUSHBACK(&threadtable, thread);

    thread->lock = LOCKINIT;
    thread->yieldawait = LOCKINIT;
    thread->stacks = (typeof(thread->stacks))LIST_INIT;

    void *stackphys = pmm_alloc(KTHREADSTACKSIZE / PAGE_SIZE);
    LIST_PUSHBACK(&thread->stacks, stackphys);
    void *stack = stackphys + KTHREADSTACKSIZE + HIGHER_HALF;

    thread->state = (cpustate_t) {
        .cs = 0x28,
        .ds = 0x30,
        .es = 0x30,
        .ss = 0x30,
        .rflags = 0x202,
        .rip = (uint64_t)func,
        .rdi = (uint64_t)arg,
        .rbp = 0,
        .rsp = (uint64_t)stack
    };

    thread->cr3 = (uint64_t)kernelpagemap->top;
    thread->timeslice = 5000;
    thread->curcpu = -1;

    if(enqueue) {
        sched_enqueue(thread);
    }

    return thread;
}
