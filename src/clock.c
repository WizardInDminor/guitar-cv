#include "clock.h"

/*
 * Register definitions (STM32F407). Inline-register style, matching
 * src/spi2.c. Addresses/bit fields per docs/reference/register-map.md.
 */
#define RCC_CR      (*(volatile uint32_t *)0x40023800)
#define RCC_PLLCFGR (*(volatile uint32_t *)0x40023804)
#define RCC_CFGR    (*(volatile uint32_t *)0x40023808)
#define RCC_APB1ENR (*(volatile uint32_t *)0x40023840) /* bit 28 PWREN */

#define FLASH_ACR   (*(volatile uint32_t *)0x40023C00)
#define PWR_CR      (*(volatile uint32_t *)0x40007000)

/* RCC_CR bits. */
#define CR_HSION    (1u << 0)
#define CR_HSIRDY   (1u << 1)
#define CR_HSEON    (1u << 16)
#define CR_HSERDY   (1u << 17)
#define CR_PLLON    (1u << 24)
#define CR_PLLRDY   (1u << 25)

/* RCC_CFGR fields. */
#define CFGR_SW_MASK    (0x3u << 0)   /* SYSCLK source select          */
#define CFGR_SW_HSI     (0x0u << 0)
#define CFGR_SW_PLL     (0x2u << 0)
#define CFGR_SWS_MASK   (0x3u << 2)   /* SYSCLK source status          */
#define CFGR_SWS_HSI    (0x0u << 2)
#define CFGR_SWS_PLL    (0x2u << 2)
#define CFGR_HPRE_MASK  (0xFu << 4)   /* AHB prescaler; 0xxx = /1      */
#define CFGR_PPRE1_MASK (0x7u << 10)  /* APB1 prescaler                */
#define CFGR_PPRE1_DIV4 (0x5u << 10)  /* 101 = /4                      */
#define CFGR_PPRE2_MASK (0x7u << 13)  /* APB2 prescaler                */
#define CFGR_PPRE2_DIV2 (0x4u << 13)  /* 100 = /2                      */

/* RCC_PLLCFGR fields. */
#define PLLCFGR_SRC_HSE (1u << 22)

/* FLASH_ACR fields. */
#define ACR_LATENCY_MASK 0xFu
#define ACR_PRFTEN       (1u << 8)
#define ACR_ICEN         (1u << 9)
#define ACR_DCEN         (1u << 10)

/* PWR_CR: VOS = 1 (scale 1) required for 168 MHz operation. */
#define PWR_CR_VOS       (1u << 14)

/* Target configuration: HSE 8 MHz -> SYSCLK 168 MHz, PLL48 = 48 MHz. */
#define PLL_M 8u
#define PLL_N 336u
#define PLL_P 2u
#define PLL_Q 7u

#define AHB_DIV  1u
#define APB1_DIV 4u   /* PCLK1 42 MHz (max 42)  */
#define APB2_DIV 2u   /* PCLK2 84 MHz (max 84)  */

#define FLASH_WAIT_STATES 5u  /* 150 < HCLK <= 168 MHz at 2.7-3.6 V */

/*
 * Bounded-wait iteration limit. SysTick is not running yet, so this is a
 * crude loop bound: ~2M iterations is hundreds of ms at 16 MHz — far beyond
 * HSE crystal startup (ms) and PLL lock (~hundreds of µs).
 */
#define WAIT_ITERS 2000000u

static void set_hsi_frequencies(clock_frequencies_t *f)
{
    f->source_hz   = CLOCK_HSI_HZ;
    f->sysclk_hz   = CLOCK_HSI_HZ;
    f->hclk_hz     = CLOCK_HSI_HZ;
    f->pclk1_hz    = CLOCK_HSI_HZ;
    f->pclk2_hz    = CLOCK_HSI_HZ;
    f->tim_apb1_hz = CLOCK_HSI_HZ;  /* APB prescalers /1 -> no doubling */
    f->tim_apb2_hz = CLOCK_HSI_HZ;
    f->pll48_hz    = 0;             /* PLL not running                  */
}

/* Reset defaults: everything on 16 MHz HSI, prescalers /1. */
static clock_frequencies_t g_clocks = {
    .source_hz   = CLOCK_HSI_HZ,
    .sysclk_hz   = CLOCK_HSI_HZ,
    .hclk_hz     = CLOCK_HSI_HZ,
    .pclk1_hz    = CLOCK_HSI_HZ,
    .pclk2_hz    = CLOCK_HSI_HZ,
    .tim_apb1_hz = CLOCK_HSI_HZ,
    .tim_apb2_hz = CLOCK_HSI_HZ,
    .pll48_hz    = 0,
};

static bool flag_becomes_set(volatile uint32_t *reg, uint32_t mask)
{
    for (uint32_t i = 0; i < WAIT_ITERS; i++) {
        if (*reg & mask) {
            return true;
        }
    }
    return false;
}

static bool sysclk_switches_to(uint32_t sws_value)
{
    for (uint32_t i = 0; i < WAIT_ITERS; i++) {
        if ((RCC_CFGR & CFGR_SWS_MASK) == sws_value) {
            return true;
        }
    }
    return false;
}

/*
 * Return to a safe HSI state after a failed transition: SYSCLK on HSI,
 * prescalers /1, PLL and HSE off. Flash latency is left high — extra wait
 * states are safe at 16 MHz, just slower. Reported frequencies then
 * describe this HSI state, so SysTick/driver init still get true values.
 */
static void fall_back_to_hsi(void)
{
    RCC_CFGR &= ~CFGR_SW_MASK;                     /* SW = HSI          */
    (void)sysclk_switches_to(CFGR_SWS_HSI);        /* best effort       */
    RCC_CFGR &= ~(CFGR_HPRE_MASK | CFGR_PPRE1_MASK | CFGR_PPRE2_MASK);
    RCC_CR   &= ~(CR_PLLON | CR_HSEON);
    set_hsi_frequencies(&g_clocks);
}

clock_status_t clock_init(void)
{
    if (!clock_pll_cfg_valid(CLOCK_HSE_HZ, PLL_M, PLL_N, PLL_P, PLL_Q)) {
        return CLOCK_ERR_INVALID_CONFIG;
    }

    /* 1. Known safe state: run from HSI, PLL off, while reconfiguring. */
    RCC_CR |= CR_HSION;
    if (!flag_becomes_set(&RCC_CR, CR_HSIRDY)) {
        return CLOCK_ERR_SWITCH_TIMEOUT;
    }
    RCC_CFGR &= ~CFGR_SW_MASK;                     /* SW = HSI          */
    if (!sysclk_switches_to(CFGR_SWS_HSI)) {
        return CLOCK_ERR_SWITCH_TIMEOUT;
    }
    RCC_CR &= ~CR_PLLON;

    /* 2-3. Power interface clock on, voltage scale 1 (needed for 168 MHz).
     *      VOS must be set while the PLL is off. */
    RCC_APB1ENR |= (1u << 28);                     /* PWREN             */
    PWR_CR |= PWR_CR_VOS;

    /* 4-5. Flash: 5 wait states + prefetch + I/D caches, BEFORE any fast
     *      clock. RM0090 requires reading the latency back to confirm. */
    FLASH_ACR = FLASH_WAIT_STATES | ACR_PRFTEN | ACR_ICEN | ACR_DCEN;
    if ((FLASH_ACR & ACR_LATENCY_MASK) != FLASH_WAIT_STATES) {
        return CLOCK_ERR_INVALID_CONFIG;
    }

    /* 6-7. HSE on, bounded wait for the crystal. */
    RCC_CR |= CR_HSEON;
    if (!flag_becomes_set(&RCC_CR, CR_HSERDY)) {
        fall_back_to_hsi();
        return CLOCK_ERR_HSE_TIMEOUT;
    }

    /* 8. Bus prescalers BEFORE the switch so APB1/APB2 are never
     *    overclocked during the transition: AHB /1, APB1 /4, APB2 /2. */
    RCC_CFGR = (RCC_CFGR & ~(CFGR_HPRE_MASK | CFGR_PPRE1_MASK | CFGR_PPRE2_MASK))
             | CFGR_PPRE1_DIV4 | CFGR_PPRE2_DIV2;

    /* 9. PLL: HSE /M ×N /P -> SYSCLK; /Q -> 48 MHz domain.
     *    PLLP field encodes /2,/4,/6,/8 as 0..3. */
    RCC_PLLCFGR = PLL_M
                | (PLL_N << 6)
                | (((PLL_P / 2u) - 1u) << 16)
                | PLLCFGR_SRC_HSE
                | (PLL_Q << 24);

    /* 10-11. PLL on, bounded wait for lock. */
    RCC_CR |= CR_PLLON;
    if (!flag_becomes_set(&RCC_CR, CR_PLLRDY)) {
        fall_back_to_hsi();
        return CLOCK_ERR_PLL_TIMEOUT;
    }

    /* 12-13. Switch SYSCLK to the PLL and verify the mux reports it. */
    RCC_CFGR = (RCC_CFGR & ~CFGR_SW_MASK) | CFGR_SW_PLL;
    if (!sysclk_switches_to(CFGR_SWS_PLL)) {
        fall_back_to_hsi();
        return CLOCK_ERR_SWITCH_TIMEOUT;
    }

    /* 14-15. Publish the frequencies now in effect (the project's
     *        SystemCoreClock equivalent — consumed via clock_get_frequencies). */
    g_clocks.source_hz   = CLOCK_HSE_HZ;
    g_clocks.sysclk_hz   = clock_pll_sysclk_hz(CLOCK_HSE_HZ, PLL_M, PLL_N, PLL_P);
    g_clocks.hclk_hz     = g_clocks.sysclk_hz / AHB_DIV;
    g_clocks.pclk1_hz    = g_clocks.hclk_hz / APB1_DIV;
    g_clocks.pclk2_hz    = g_clocks.hclk_hz / APB2_DIV;
    g_clocks.tim_apb1_hz = clock_apb_timer_hz(g_clocks.pclk1_hz, APB1_DIV);
    g_clocks.tim_apb2_hz = clock_apb_timer_hz(g_clocks.pclk2_hz, APB2_DIV);
    g_clocks.pll48_hz    = clock_pll48_hz(CLOCK_HSE_HZ, PLL_M, PLL_N, PLL_Q);

    return CLOCK_OK;
}

const clock_frequencies_t *clock_get_frequencies(void)
{
    return &g_clocks;
}
