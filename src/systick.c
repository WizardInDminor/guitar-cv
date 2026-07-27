#include "systick.h"

/*
 * Register definitions (Cortex-M4 core, not STM32-specific). Inline-register
 * style, matching src/spi2.c. Addresses per ARMv7-M ARM, SysTick section.
 */
#define SYST_CSR (*(volatile uint32_t *)0xE000E010) /* control and status */
#define SYST_RVR (*(volatile uint32_t *)0xE000E014) /* reload value       */
#define SYST_CVR (*(volatile uint32_t *)0xE000E018) /* current value      */

/* SYST_CSR bits. */
#define CSR_ENABLE    (1u << 0)  /* counter enable                  */
#define CSR_TICKINT   (1u << 1)  /* exception on count-to-zero      */
#define CSR_CLKSOURCE (1u << 2)  /* 1 = processor clock (HCLK)      */

static volatile uint32_t tick_ms;

/*
 * SysTick exception handler (vector table slot 15). Bounded bookkeeping
 * only — no application logic belongs here.
 */
void SysTick_Handler(void)
{
    tick_ms++;
}

void systick_init(uint32_t system_core_clock_hz)
{
    /* 1 ms tick; reload math in systick_reload_1ms() (host-tested). */
    SYST_CSR = 0;                                  /* stop during reconfig  */
    SYST_RVR = systick_reload_1ms(system_core_clock_hz);
    SYST_CVR = 0;                                  /* write clears counter  */
    SYST_CSR = CSR_CLKSOURCE | CSR_TICKINT | CSR_ENABLE;
}

uint32_t millis(void)
{
    return tick_ms;
}

bool time_elapsed(uint32_t start, uint32_t interval_ms)
{
    return systick_interval_elapsed(millis(), start, interval_ms);
}

void delay_ms(uint32_t ms)
{
    uint32_t start = millis();

    while (!time_elapsed(start, ms)) { }
}
