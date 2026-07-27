/*
 * Host-side unit tests for the pure clock/timing math.
 *
 * Covers the PLL/bus-frequency helpers (clock.h), the SPI baud-divider
 * selection (spi2.h), and the I2C standard-mode timing math (i2c1.h) — all
 * static inline pure functions with no register access. The register
 * transition sequence in src/clock.c is verified on the bench.
 */
#include <stdio.h>
#include <stdint.h>

#include "clock.h"
#include "spi2.h"
#include "i2c1.h"

static int failures = 0;
static int checks   = 0;

#define CHECK_EQ(actual, expected, label)                                     \
    do {                                                                      \
        unsigned long _a = (unsigned long)(actual);                           \
        unsigned long _e = (unsigned long)(expected);                         \
        checks++;                                                             \
        if (_a != _e) {                                                       \
            failures++;                                                       \
            printf("FAIL %-40s got %lu, expected %lu\n", (label), _a, _e);    \
        }                                                                     \
    } while (0)

static void test_pll_math(void)
{
    /* Target tree: HSE 8 MHz, M=8, N=336, P=2, Q=7. */
    CHECK_EQ(clock_pll_sysclk_hz(8000000u, 8, 336, 2), 168000000u, "SYSCLK 168 MHz");
    CHECK_EQ(clock_pll48_hz(8000000u, 8, 336, 7),       48000000u, "PLL48 48 MHz");

    /* Divide-first ordering: a 25 MHz HSE variant must not overflow. */
    CHECK_EQ(clock_pll_sysclk_hz(25000000u, 25, 336, 2), 168000000u, "25 MHz HSE variant");

    /* Derived bus clocks. */
    CHECK_EQ(168000000u / 1u, 168000000u, "HCLK  = SYSCLK/1");
    CHECK_EQ(168000000u / 4u,  42000000u, "PCLK1 = HCLK/4");
    CHECK_EQ(168000000u / 2u,  84000000u, "PCLK2 = HCLK/2");
}

static void test_apb_timer_doubling(void)
{
    /* APB prescaler > 1 doubles the timer kernel clock; /1 does not. */
    CHECK_EQ(clock_apb_timer_hz(42000000u, 4),  84000000u, "TIM(APB1) 2x at /4");
    CHECK_EQ(clock_apb_timer_hz(84000000u, 2), 168000000u, "TIM(APB2) 2x at /2");
    CHECK_EQ(clock_apb_timer_hz(16000000u, 1),  16000000u, "TIM 1x at /1 (HSI)");
}

static void test_pll_cfg_validation(void)
{
    /* The target configuration is legal. */
    CHECK_EQ(clock_pll_cfg_valid(8000000u, 8, 336, 2, 7), 1, "target cfg valid");

    /* Field range violations. */
    CHECK_EQ(clock_pll_cfg_valid(8000000u, 1, 336, 2, 7),  0, "M=1 invalid");
    CHECK_EQ(clock_pll_cfg_valid(8000000u, 8,  49, 2, 7),  0, "N=49 invalid");
    CHECK_EQ(clock_pll_cfg_valid(8000000u, 8, 433, 2, 7),  0, "N=433 invalid");
    CHECK_EQ(clock_pll_cfg_valid(8000000u, 8, 336, 3, 7),  0, "P=3 invalid");
    CHECK_EQ(clock_pll_cfg_valid(8000000u, 8, 336, 2, 16), 0, "Q=16 invalid");

    /* VCO input must stay in 1..2 MHz. */
    CHECK_EQ(clock_pll_cfg_valid(8000000u, 2, 336, 2, 7), 0, "VCO in 4 MHz invalid");
    CHECK_EQ(clock_pll_cfg_valid(8000000u, 16, 336, 2, 7), 0, "VCO in 0.5 MHz invalid");

    /* VCO output must stay in 100..432 MHz. */
    CHECK_EQ(clock_pll_cfg_valid(8000000u, 8, 96, 2, 7),  0, "VCO out 96 MHz invalid");

    /* SYSCLK above the 168 MHz part limit. */
    CHECK_EQ(clock_pll_cfg_valid(8000000u, 8, 432, 2, 7), 0, "SYSCLK 216 MHz invalid");
}

static void test_spi_divider(void)
{
    /* 42 MHz PCLK1, 2 MHz cap: /32 -> 1.3125 MHz (BR=4). */
    CHECK_EQ(spi_br_for_max_hz(42000000u, 2000000u), 4, "42 MHz cap 2 MHz -> BR 4");
    CHECK_EQ(spi_sck_hz(42000000u, 4), 1312500u,        "42 MHz BR 4 -> 1.3125 MHz");

    /* 16 MHz PCLK1 (HSI bring-up): /8 -> exactly 2 MHz (BR=2, per ADR-003). */
    CHECK_EQ(spi_br_for_max_hz(16000000u, 2000000u), 2, "16 MHz cap 2 MHz -> BR 2");
    CHECK_EQ(spi_sck_hz(16000000u, 2), 2000000u,        "16 MHz BR 2 -> 2 MHz");

    /* Generous cap -> fastest divider. */
    CHECK_EQ(spi_br_for_max_hz(42000000u, 100000000u), 0, "huge cap -> BR 0 (/2)");
    CHECK_EQ(spi_sck_hz(42000000u, 0), 21000000u,         "BR 0 -> 21 MHz");

    /* Slowest legal divider boundary: /256. */
    CHECK_EQ(spi_br_for_max_hz(42000000u, 164063u), 7, "cap just above /256 -> BR 7");
    CHECK_EQ(spi_sck_hz(42000000u, 7), 164062u,        "BR 7 -> 164.062 kHz");

    /* Impossible: even /256 exceeds the cap -> sentinel 8 (invalid). */
    CHECK_EQ(spi_br_for_max_hz(42000000u, 100000u), 8, "cap below /256 -> invalid");
    CHECK_EQ(spi_br_for_max_hz(42000000u, 0u),      8, "cap 0 -> invalid");
}

static void test_i2c_timing(void)
{
    /* 42 MHz PCLK1, 100 kHz standard mode. */
    CHECK_EQ(i2c_freq_field(42000000u),        42, "FREQ  @42 MHz");
    CHECK_EQ(i2c_sm_ccr(42000000u, 100000u),  210, "CCR   @42 MHz/100 kHz");
    CHECK_EQ(i2c_sm_trise(42000000u),          43, "TRISE @42 MHz");

    /* 16 MHz PCLK1 (HSI bring-up) — the previously verified values. */
    CHECK_EQ(i2c_freq_field(16000000u),        16, "FREQ  @16 MHz");
    CHECK_EQ(i2c_sm_ccr(16000000u, 100000u),   80, "CCR   @16 MHz/100 kHz");
    CHECK_EQ(i2c_sm_trise(16000000u),          17, "TRISE @16 MHz");

    /* Ceil rounding keeps SCL at or below target on non-integer ratios:
     * 42 MHz / (2 x 90 kHz) = 233.33 -> 234 -> SCL 89.7 kHz <= 90 kHz. */
    CHECK_EQ(i2c_sm_ccr(42000000u, 90000u),   234, "CCR ceil-rounds up");

    /* Boundary: minimum legal FREQ field (2 MHz PCLK). */
    CHECK_EQ(i2c_freq_field(2000000u),          2, "FREQ  @2 MHz (min)");
    CHECK_EQ(i2c_sm_ccr(2000000u, 100000u),    10, "CCR   @2 MHz");
}

static void test_systick_reload_targets(void)
{
    /* Both planned core clocks must fit the 24-bit reload field. */
    CHECK_EQ((168000000u / 1000u - 1u) <= 0x00FFFFFFu, 1, "168 MHz reload fits 24-bit");
    CHECK_EQ(168000000u / 1000u - 1u, 167999u,            "168 MHz reload value");
}

int main(void)
{
    test_pll_math();
    test_apb_timer_doubling();
    test_pll_cfg_validation();
    test_spi_divider();
    test_i2c_timing();
    test_systick_reload_targets();

    if (failures == 0) {
        printf("OK: %d checks passed\n", checks);
        return 0;
    }
    printf("FAILED: %d of %d checks failed\n", failures, checks);
    return 1;
}
