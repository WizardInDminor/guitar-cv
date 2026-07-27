# Clock Architecture — 168 MHz PLL

`src/clock.c` / `include/clock.h` are the **single source of truth for clock
frequencies**. `clock_init()` takes the MCU from the 16 MHz HSI reset state to 168 MHz
via the PLL; every driver then receives its bus clock from `clock_get_frequencies()`
instead of hard-coding rates.

## Clock Tree

```
HSE 8 MHz (DISC1 crystal X2)
  └─ /M=8 → 1 MHz PLL input          (must be 1–2 MHz)
       └─ ×N=336 → 336 MHz VCO       (must be 100–432 MHz)
            ├─ /P=2 → 168 MHz SYSCLK (F407 max)
            └─ /Q=7 →  48 MHz PLL48  (USB FS / SDIO / RNG)

SYSCLK 168 MHz
  └─ AHB /1  → HCLK 168 MHz          (core, SysTick CLKSOURCE=1, DMA)
       ├─ APB1 /4 → PCLK1 42 MHz     (SPI2, I2C1, TIM2–7, 12–14) — max 42
       │              └─ ×2 → TIM(APB1) 84 MHz
       └─ APB2 /2 → PCLK2 84 MHz     (ADC, TIM1/8–11, SPI1)      — max 84
                      └─ ×2 → TIM(APB2) 168 MHz
```

**Timer ×2 rule (RM0090):** when an APB prescaler is >1, timers on that bus run at
**twice** the peripheral bus clock. This is why `clock_frequencies_t` reports
`tim_apb1_hz` (84 MHz) separately from `pclk1_hz` (42 MHz) — the distinction matters
directly for the future 24 kHz ADC trigger.

## API

```c
clock_status_t cst = clock_init();                        /* once, first thing */
const clock_frequencies_t *clocks = clock_get_frequencies();

systick_init(clocks->hclk_hz);
dac_init(clocks->pclk1_hz);              /* SPI divider derived inside */
i2c1_init(clocks->pclk1_hz, 100000u);    /* FREQ/CCR/TRISE derived inside */
```

`clock_get_frequencies()` is valid in every state: before init (HSI defaults), after
success (PLL values), and after failure (HSI fallback values) — so downstream init is
always computed from the truth.

## Transition Sequence (safe ordering)

1. Known safe state: HSI on and selected as SYSCLK (bounded wait), PLL off.
2. `RCC_APB1ENR.PWREN` — power interface clock on.
3. `PWR_CR.VOS = 1` — voltage scale 1, required for 168 MHz (set while PLL is off).
4. `FLASH_ACR` — **5 wait states** (2.7–3.6 V, 150 < HCLK ≤ 168 MHz) plus prefetch,
   instruction cache, data cache — **before** any fast clock; latency read back to
   confirm (RM0090 requirement).
5. HSE on, bounded wait for `HSERDY`.
6. Bus prescalers set **before** the switch (AHB /1, APB1 /4, APB2 /2) so no APB bus is
   ever overclocked during the transition.
7. `RCC_PLLCFGR`: M=8, N=336, P=2 (encoded 00), Q=7, source = HSE. Constants validated
   against field/VCO limits by `clock_pll_cfg_valid()` before any hardware is touched.
8. PLL on, bounded wait for `PLLRDY`.
9. `RCC_CFGR.SW = PLL`, bounded wait until `SWS` confirms.
10. Publish frequencies (the project's `SystemCoreClock` equivalent — there is no CMSIS
    `SystemCoreClock` variable in this bare-metal tree; the struct is the authority).

All waits are bounded loop counts (~2M iterations ≫ HSE crystal startup and PLL lock
times); **nothing spins forever**.

## Failure Behavior

There is no silent fallback. On HSE timeout, PLL-lock timeout, or switch timeout,
`clock_init()`:

- returns `CLOCK_ERR_HSE_TIMEOUT` / `CLOCK_ERR_PLL_TIMEOUT` / `CLOCK_ERR_SWITCH_TIMEOUT`,
- restores a safe HSI configuration (SYSCLK = HSI, prescalers /1, PLL and HSE off;
  flash latency left high, which is safe at 16 MHz),
- reports the **HSI frequencies** so SysTick and drivers still initialize correctly.

`main()` signals the fault visibly: **100 ms LED blink** = clock fault (running on HSI
fallback); **250 ms blink** = a driver rejected its derived timing
(`SPI_ERR_INVALID_CONFIG` / `I2C_ERR_INVALID_CONFIG`); **500 ms** = normal demo
heartbeat.

## Effects on Existing Drivers

| Driver | Before (16 MHz hardcoded) | After (derived from PCLK1 = 42 MHz) |
|---|---|---|
| SPI2 (MCP4922) | BR=÷8 → 2 MHz | Fastest divider ≤ 2 MHz cap → **÷32 = 1.3125 MHz** |
| I2C1 (SSD1306) | FREQ=16, CCR=80, TRISE=17 | **FREQ=42, CCR=210, TRISE=43** (still 100 kHz SM) |
| SysTick | reload 15,999 | reload **167,999** (fits 24-bit) |

The SPI 2 MHz cap is ADR-003's breadboard policy, now enforced as a cap rather than a
fixed divider. All three derivations are pure functions, host-tested in
`test/test_clock.c`.

## Future ADC Trigger (Phase 2)

The 24 kHz ADC sample trigger will come from an APB1 timer (e.g. TIM2/TIM3) clocked at
`clocks->tim_apb1_hz` = **84 MHz**, *not* PCLK1's 42 MHz — the ×2 rule above. The math
lands exactly: 84 MHz / 24 kHz = **3500**, e.g. prescaler 0 with auto-reload 3499. The
ADC itself is on APB2 (`pclk2_hz`, ADCCLK prescaler decided in the ADC ADR).

## Bench Verification Checklist (pending)

1. Firmware still boots; no fault blink.
2. Heartbeat still 500 ms on / 500 ms off (proves the 1 ms tick at 168 MHz).
3. Saleae: SPI2 frames decode; measure SCK ≈ **1.3125 MHz**.
4. DAC walk still reads ~0 / 1.000 / 2.000 V.
5. Saleae: I2C init sequence decodes at address 0x3D; measure SCL ≈ **100 kHz**.
6. OLED still reaches solid white.
7. Pull the HSE path (or misconfigure M) and confirm the 100 ms fault blink appears
   and the board keeps running on HSI.
8. Optional: route MCO2 (PC9, currently unused) to output SYSCLK/4 or /5 and measure
   directly (RCC_CFGR.MCO2/MCO2PRE); keep ≤ ~50 MHz for probe fidelity.
