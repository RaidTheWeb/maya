#include <limine.h>
#include <stdint.h>
#include <stddef.h>


static volatile struct limine_terminal_request termreq = {
    .id = LIMINE_TERMINAL_REQUEST,
    .revision = 0
};

void _start(void) {
    if(termreq.response == NULL || termreq.response->terminal_count < 1) for(;;) asm("hlt");

    struct limine_terminal *term = termreq.response->terminals[0];
    termreq.response->write(term, "Hello World!", 12);

    for(;;) asm("hlt");
}

