/*
 * Host-side unit tests for the pure SysTick interval logic.
 *
 * systick_interval_elapsed() is the rollover-safe comparison behind
 * time_elapsed(); it is a static inline in systick.h with no hardware
 * access, so it tests on the host with `make test`. The tick ISR, register
 * init, and millis() are register I/O and are verified on the bench.
 */
#include <stdio.h>
#include <stdint.h>

#include "systick.h"

static int failures = 0;
static int checks   = 0;

#define CHECK(cond, label)                                                    \
    do {                                                                      \
        checks++;                                                             \
        if (!(cond)) {                                                        \
            failures++;                                                       \
            printf("FAIL %s\n", (label));                                     \
        }                                                                     \
    } while (0)

static void test_basic_intervals(void)
{
    /* Not yet elapsed. */
    CHECK(!systick_interval_elapsed(100, 100, 1),   "same instant, 1ms");
    CHECK(!systick_interval_elapsed(109, 100, 10),  "9 of 10 ms");

    /* Exactly elapsed and beyond. */
    CHECK(systick_interval_elapsed(110, 100, 10),   "exactly 10 ms");
    CHECK(systick_interval_elapsed(111, 100, 10),   "11 of 10 ms");

    /* Zero interval is always elapsed. */
    CHECK(systick_interval_elapsed(100, 100, 0),    "zero interval");
}

static void test_rollover(void)
{
    /* start near wrap, now wrapped past zero: 0xFFFFFFF6 + 20 = 0xA. */
    CHECK(!systick_interval_elapsed(0x00000005u, 0xFFFFFFF6u, 20), "wrap, 15 of 20 ms");
    CHECK( systick_interval_elapsed(0x0000000Au, 0xFFFFFFF6u, 20), "wrap, exactly 20 ms");
    CHECK( systick_interval_elapsed(0x00000064u, 0xFFFFFFF6u, 20), "wrap, well past");

    /* start at the maximum tick value. */
    CHECK(!systick_interval_elapsed(0xFFFFFFFFu, 0xFFFFFFFFu, 1), "start at max, same tick");
    CHECK( systick_interval_elapsed(0x00000000u, 0xFFFFFFFFu, 1), "start at max, +1 wraps");

    /* A naive signed/relational compare would fail these. */
    CHECK(!systick_interval_elapsed(0x00000000u, 0x80000000u, 0x80000001u),
          "half-range interval not yet elapsed");
    CHECK( systick_interval_elapsed(0x00000001u, 0x80000000u, 0x80000001u),
          "half-range interval elapsed");
}

int main(void)
{
    test_basic_intervals();
    test_rollover();

    if (failures == 0) {
        printf("OK: %d checks passed\n", checks);
        return 0;
    }
    printf("FAILED: %d of %d checks failed\n", failures, checks);
    return 1;
}
