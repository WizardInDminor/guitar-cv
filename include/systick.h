#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>
#include <stdbool.h>

/*
 * SysTick 1 ms monotonic timebase.
 *
 * The core-clock frequency is passed in at init so the module works unchanged
 * on the 16 MHz HSI default and after the 168 MHz PLL bring-up: the caller
 * re-runs systick_init() with the new frequency after any clock change.
 *
 * millis() rolls over every ~49.7 days. All interval comparisons must go
 * through time_elapsed() (or systick_interval_elapsed()), whose unsigned
 * subtraction is rollover-safe. Never compare millis() values with < or >.
 */

/* Start the 1 ms tick. Safe to call again after a core-clock change. */
void systick_init(uint32_t system_core_clock_hz);

/* Milliseconds since systick_init(). Monotonic, wraps at 2^32. */
uint32_t millis(void);

/* True once at least interval_ms have passed since start (a millis() value). */
bool time_elapsed(uint32_t start, uint32_t interval_ms);

/*
 * Blocking wait. For initialization and controlled bring-up code ONLY —
 * application scheduling must use non-blocking time_elapsed() checks.
 */
void delay_ms(uint32_t ms);

/*
 * Pure rollover-safe interval check: true when (now - start), evaluated in
 * modulo-2^32 arithmetic, has reached interval_ms. No hardware access —
 * unit-tested on the host (test/test_systick.c); time_elapsed() is this
 * with now = millis().
 */
static inline bool systick_interval_elapsed(uint32_t now, uint32_t start,
                                            uint32_t interval_ms)
{
    return (uint32_t)(now - start) >= interval_ms;
}

#endif /* SYSTICK_H */
