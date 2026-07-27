# Timing Subsystem — SysTick

`src/systick.c` / `include/systick.h` provide the platform's 1 ms monotonic timebase
using the Cortex-M4 core SysTick timer. Bare-metal, register-level, no HAL — consistent
with the rest of the firmware.

## API

```c
void     systick_init(uint32_t system_core_clock_hz);
uint32_t millis(void);
bool     time_elapsed(uint32_t start, uint32_t interval_ms);
void     delay_ms(uint32_t ms);   /* init / bring-up code ONLY */
```

Usage pattern for application scheduling (non-blocking, drift-free):

```c
uint32_t last = millis();

while (1) {
    if (time_elapsed(last, 500u)) {
        last += 500u;          /* advance by the interval, not to "now" */
        do_periodic_work();
    }
    /* loop stays free for other work */
}
```

`delay_ms()` is a blocking spin on `time_elapsed()` and is reserved for initialization
and controlled bring-up sequencing (e.g. the SSD1306 power-on settle). Application
logic must never block on it.

**Missed-interval policy is per subsystem, not baked into the timing layer.** The `if`
form above processes one elapsed event per loop pass — if the loop stalls for several
intervals, extra events are dropped and cadence resumes cleanly (right for UI refresh,
heartbeats). Subsystems that must not lose ticks (e.g. sequencer tempo) can use
`while (time_elapsed(last, interval)) { last += interval; process_one_tick(); }` to
catch up, accepting a burst of work after a stall. Choose deliberately per subsystem.

## Registers

SysTick is a Cortex-M core peripheral (ARMv7-M ARM), not an STM32 peripheral — the
registers live at `0xE000E010`, outside the STM32 bus matrix:

| Register | Address | Use |
|---|---|---|
| `SYST_CSR` | 0xE000E010 | `CLKSOURCE=1` (HCLK), `TICKINT=1`, `ENABLE=1` |
| `SYST_RVR` | 0xE000E014 | Reload = clocks-per-ms − 1 |
| `SYST_CVR` | 0xE000E018 | Any write clears the counter |

**Reload calculation:** `reload = core_clock_hz / 1000 − 1` — the counter counts
`reload…0` inclusive, hence the −1.

| Core clock | Reload | Fits 24-bit (max 16,777,215)? |
|---|---|---|
| 16 MHz HSI (current) | 15,999 | ✅ |
| 168 MHz PLL (planned) | 167,999 | ✅ |

## ISR Contract

`SysTick_Handler` (vector table slot 15, wired in `startup/startup_stm32f407.s`)
performs bounded tick bookkeeping only — it increments the millisecond counter and
returns. No application scheduling logic goes in the ISR.

## Rollover Safety

`millis()` wraps every ~49.7 days (2³²  ms). All interval checks go through the pure
helper:

```c
static inline bool systick_interval_elapsed(uint32_t now, uint32_t start,
                                            uint32_t interval_ms)
{
    return (uint32_t)(now - start) >= interval_ms;
}
```

Unsigned modulo-2³² subtraction makes `now − start` correct across the wrap; naive
`now >= start + interval` comparisons are not. Never compare raw `millis()` values
with relational operators. This helper is host-tested in `test/test_systick.c`,
including wrap-around cases.

## Clock Source

SysTick is initialized from the [clock layer](clock.md)'s reported HCLK:

```c
clock_init();                        /* HSI -> 168 MHz PLL (or HSI fallback) */
const clock_frequencies_t *clocks = clock_get_frequencies();
systick_init(clocks->hclk_hz);       /* reload 167,999 @ 168 MHz; 15,999 @ 16 MHz */
```

`CLKSOURCE=1` runs SysTick from HCLK, and the clock layer reports HCLK explicitly, so
the reload is correct in every state — full-speed PLL or HSI fallback after a clock
fault. Re-running `systick_init()` after any future clock change stops the counter,
reloads it for the new frequency, and restarts it; tick continuity across the switch is
not guaranteed (one tick may stretch), which is acceptable during initialization.
