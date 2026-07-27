#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Central clock layer — the single source of truth for bus frequencies.
 *
 * clock_init() takes the MCU from the 16 MHz HSI reset state to 168 MHz via
 * the PLL (HSE 8 MHz on the DISC1). Drivers must not hard-code clock values:
 * they receive the relevant frequency from clock_get_frequencies() at init.
 *
 * Target tree (see docs/firmware/clock.md):
 *   HSE 8 MHz /M=8 -> 1 MHz ×N=336 -> VCO 336 MHz /P=2 -> SYSCLK 168 MHz
 *                                              VCO /Q=7 -> 48 MHz (USB/SDIO/RNG)
 *   AHB /1 -> HCLK 168 MHz; APB1 /4 -> PCLK1 42 MHz; APB2 /2 -> PCLK2 84 MHz
 *   Timer clocks: APB prescaler > 1 doubles the timer clock, so
 *   TIM(APB1) = 84 MHz and TIM(APB2) = 168 MHz.
 *
 * On any failure (HSE, PLL lock, or switch timeout) the MCU is returned to a
 * safe HSI configuration, the reported frequencies reflect HSI, and a
 * specific error is returned — there is no silent fallback.
 */

#define CLOCK_HSI_HZ 16000000u  /* internal RC, reset default        */
#define CLOCK_HSE_HZ  8000000u  /* DISC1 external crystal (X2, 8 MHz) */

typedef struct {
    uint32_t source_hz;    /* PLL input source (HSE), or HSI on fallback */
    uint32_t sysclk_hz;
    uint32_t hclk_hz;      /* AHB / core / SysTick (CLKSOURCE=1)         */
    uint32_t pclk1_hz;     /* APB1: SPI2, I2C1, TIM2..7 bus clock        */
    uint32_t pclk2_hz;     /* APB2: ADC, TIM1/8..11 bus clock            */
    uint32_t tim_apb1_hz;  /* APB1 timer kernel clock (2× when PPRE1>1)  */
    uint32_t tim_apb2_hz;  /* APB2 timer kernel clock (2× when PPRE2>1)  */
    uint32_t pll48_hz;     /* PLL Q output (USB/SDIO/RNG); 0 when no PLL */
} clock_frequencies_t;

typedef enum {
    CLOCK_OK = 0,
    CLOCK_ERR_HSE_TIMEOUT,     /* HSE never became ready                 */
    CLOCK_ERR_PLL_TIMEOUT,     /* PLL never locked                       */
    CLOCK_ERR_SWITCH_TIMEOUT,  /* SYSCLK mux never reported the switch   */
    CLOCK_ERR_INVALID_CONFIG,  /* PLL constants out of range / flash ACR */
} clock_status_t;

/* Run the HSI -> PLL transition. Safe to call once, early in main(). */
clock_status_t clock_init(void);

/*
 * Frequencies actually in effect. Valid even after a failed clock_init()
 * (they then describe the HSI fallback state) and before it (HSI defaults).
 */
const clock_frequencies_t *clock_get_frequencies(void);

/*
 * Pure calculation helpers — no hardware access, unit-tested on the host
 * (test/test_clock.c). Divide-first ordering keeps the math in 32 bits.
 */

/* f_SYSCLK = src / M × N / P */
static inline uint32_t clock_pll_sysclk_hz(uint32_t src_hz, uint32_t m,
                                           uint32_t n, uint32_t p)
{
    return src_hz / m * n / p;
}

/* f_PLL48 = src / M × N / Q */
static inline uint32_t clock_pll48_hz(uint32_t src_hz, uint32_t m,
                                      uint32_t n, uint32_t q)
{
    return src_hz / m * n / q;
}

/* STM32F4 rule: timers on an APB bus run at 2× PCLK when that APB
 * prescaler is > 1, at PCLK when it is 1. */
static inline uint32_t clock_apb_timer_hz(uint32_t pclk_hz, uint32_t apb_div)
{
    return (apb_div > 1u) ? (pclk_hz * 2u) : pclk_hz;
}

/* Validate PLL constants against RM0090 field and VCO limits. */
static inline bool clock_pll_cfg_valid(uint32_t src_hz, uint32_t m,
                                       uint32_t n, uint32_t p, uint32_t q)
{
    if (m < 2u || m > 63u)   return false;
    if (n < 50u || n > 432u) return false;
    if (q < 2u || q > 15u)   return false;
    if (p != 2u && p != 4u && p != 6u && p != 8u) return false;

    uint32_t vco_in = src_hz / m;          /* must be 1..2 MHz    */
    if (vco_in < 1000000u || vco_in > 2000000u) return false;

    uint32_t vco_out = vco_in * n;         /* must be 100..432 MHz */
    if (vco_out < 100000000u || vco_out > 432000000u) return false;

    if (vco_out / p > 168000000u) return false;  /* F407 SYSCLK cap */

    return true;
}

#endif /* CLOCK_H */
