#include <Arduino.h>

#include "timer_module.h"

void reset_timer(struct timer_t *tmr, uint64_t milliseconds)
{
    tmr->prev_time_ms = millis();
    tmr->current_time_ms = tmr->prev_time_ms;
    tmr->elapsed_time = milliseconds;
    tmr->timeout = RESET_TIMER_VALUE;
}

void timer_timeout(struct timer_t *tmr)
{
    tmr->current_time_ms = millis();
    
    if ((tmr->current_time_ms - tmr->prev_time_ms) >= tmr->elapsed_time) {
        tmr->timeout = true;
        tmr->prev_time_ms = tmr->current_time_ms;
    }
}