#ifndef __SYS__TIME_H__
#define __SYS__TIME_H__

#include <stdint.h>
#include <limine.h>

#define TIMERFREQ 1000
#define CLOCKREAL 0
#define CLOCKMONO 1

typedef struct {
    int64_t tvsec;
    int64_t tvnsec;
} timespec_t;

extern timespec_t monoclock;
extern timespec_t realclock;

void timespec_add(timespec_t *timespec, timespec_t interval);
int timespec_sub(timespec_t *timespec, timespec_t interval);


void time_init(struct limine_boot_time_response *boottime);

// timer_t

#endif
