#ifndef TIMER_MODULE_H
#define TIMER_MODULE_H

#include <stdint.h>
#include <stdbool.h>

#define RESET_TIMER_VALUE false

struct timer_t {
    uint64_t prev_time_ms;
    uint64_t current_time_ms;
    uint64_t elapsed_time;
    bool timeout;
};

void reset_timer(struct timer_t *tmr, uint64_t milliseconds);
void timer_timeout(struct timer_t *tmr);

#endif /* TIMER_MODULE_H */