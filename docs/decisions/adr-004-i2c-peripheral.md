# ADR-004 — I2C Peripheral Selection

**Date:** 2026-06-06  
**Status:** Accepted  
**Supersedes:** —  
**Superseded by:** —

---

## Context

The SSD1306 OLED (128×64, see [Bill of Materials](../hardware/bom.md)) is driven over
I2C. The STM32F407VGT6 has three I2C peripherals: I2C1, I2C2, and I2C3. We need to select
which one to use and which physical pins to assign.

As with the SPI selection ([ADR-001](adr-001-spi-peripheral.md)), the DISC1 development
board has onboard peripherals that already consume some I2C-capable pins, so the schematic
governs the choice. The same discipline applies: confirm pins are free and routed to the
expansion headers before committing.

Key constraints:

- Bare-metal register-level development — no HAL pin-conflict detection
- Must not interfere with onboard DISC1 peripherals
- Pins must be available on the expansion headers for breadboard access
- I2C requires open-drain outputs and pull-ups (different GPIO config from SPI)

---

## Decision

Use **I2C2 on PB10 (SCL) and PB11 (SDA)**, alternate function **AF4**, in standard mode
(100 kHz) for bring-up.

---

## Options Considered

### Option A — I2C1 (PB6/PB7 or PB8/PB9)
I2C1 is the conventional first choice and sits on APB1.

**Pros:**
- The "default" I2C peripheral in most examples

**Cons:**
- The DISC1 routes **PB6 (SCL) / PB9 (SDA)** to the **CS43L22 audio codec** control
  interface (confirmed in the [Session 02](../sessions/session-02.md) schematic trace)
- Using I2C1 on these pins would share the bus with the onboard codec — avoidable
  contention for a bring-up bus

### Option B — I2C2 (PB10/PB11)
I2C2 sits on APB1, alongside SPI2.

**Pros:**
- PB10/PB11 are free — not connected to any onboard DISC1 peripheral
- Both pins are on the same GPIOB port as the existing SPI2 pins (PB12–PB15) and on the
  P1 expansion header — logical grouping, no new port to clock
- AF4 applies uniformly to both pins — simple configuration
- Same clock domain (APB1) and timing reasoning as the SPI driver

**Cons:**
- None material for this use case

### Option C — I2C3 (PA8/PC9)
I2C3 also sits on APB1.

**Pros:**
- An available fallback if I2C2 were occupied

**Cons:**
- Spans two ports (PA8 + PC9) — two GPIO ports to clock and configure
- Pins would need separate schematic verification
- Not investigated in detail once I2C2 was confirmed free

---

## Rationale

I2C1 was eliminated because its DISC1 pins are consumed by the onboard audio codec — the
same class of conflict that pushed SPI onto SPI2 in ADR-001. I2C2 was selected because
PB10/PB11 are free, sit on the same port as the already-routed SPI2 pins, and use a single
uniform alternate function (AF4). I2C3 was not evaluated further once I2C2 was confirmed
available. The choice keeps both serial buses (SPI2 + I2C2) contiguous on GPIOB and on the
same APB1 clock domain, simplifying the mental model and the wiring.

This decision was grounded in hardware evidence (schematic trace) rather than assumption.

---

## Consequences

- All I2C driver code targets I2C2 at base address 0x40005800
- GPIOB must be clocked (RCC_AHB1ENR bit 1, already on for SPI2) and I2C2 must be clocked
  (RCC_APB1ENR bit 22)
- PB10 and PB11 require AF4 configuration via GPIOB_AFRH, **open-drain** (OTYPER=1) with
  pull-ups (internal PUPDR=01, or external/module 4.7 kΩ)
- I2C timing registers (CR2.FREQ, CCR, TRISE) are derived from the **current 16 MHz APB1**
  clock; they must be recomputed when the PLL raises APB1 in a later session
  (cf. [ADR-003](adr-003-spi-clock-rate.md))
- If I2C2 is needed for another peripheral in the future, this decision must be revisited

---

## References

- STM32F407 Reference Manual — I2C peripheral, Section 27
- STM32F407 Datasheet — Alternate function mapping table (I2C2 = AF4)
- DISC1 MB997E Schematic — CS43L22 codec on PB6/PB9
- [I2C Peripheral Deep Dive](../concepts/i2c-peripheral.md)
- [SSD1306 OLED Driver](../firmware/ssd1306-driver.md)
- [ADR-001 — SPI Peripheral Selection](adr-001-spi-peripheral.md)
