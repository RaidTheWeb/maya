#include <sys/time.h>
#include <sys/pit.h>
#include <lib/stdio.h>

timespec_t monoclock;
timespec_t realclock;

void timespec_add(timespec_t *timespec, timespec_t interval) {
    if(timespec->tvnsec + interval.tvnsec > 999999999) {
        uint64_t diff = (timespec->tvnsec + interval.tvnsec) - 1000000000;
        timespec->tvnsec = diff;
        timespec->tvsec++;
    } else timespec->tvnsec += interval.tvnsec;

    timespec->tvsec += interval.tvsec;
}

int timespec_sub(timespec_t *timespec, timespec_t interval) {
    if(interval.tvnsec > timespec->tvnsec) {
        uint64_t diff = interval.tvnsec - timespec->tvnsec;
        timespec->tvnsec = 999999999 - diff;
        if(!timespec->tvsec) {
            timespec->tvsec = 0;
            timespec->tvnsec = 0;
        }
        timespec->tvsec--;
    } else timespec->tvnsec -= interval.tvnsec;

    if(interval.tvnsec > timespec->tvsec) {
        timespec->tvsec = 0;
        timespec->tvnsec = 0;
        return 1;
    }
    timespec->tvsec -= interval.tvsec;
    if(!timespec->tvsec && !timespec->tvnsec) return 1;
    return 0;
}

void time_init(struct limine_boot_time_response *boottime) {
    int64_t epoch = boottime->boot_time;

    monoclock = (timespec_t) { .tvsec = epoch, .tvnsec = 0 };
    realclock = (timespec_t) { .tvsec = epoch, .tvnsec = 0 };

    pit_init();
}

void timer_handler(void) {
    printf("timer handler\n");
}
