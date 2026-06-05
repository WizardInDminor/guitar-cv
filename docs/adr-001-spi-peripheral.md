# ADR-001 — SPI Peripheral Selection

**Date:** 2026-04-10  
**Status:** Accepted  
**Supersedes:** —  
**Superseded by:** —

---

## Context

The STM32F407VGT6 has three SPI peripherals: SPI1, SPI2, and SPI3. The MCP4922 DAC requires an SPI master. We need to select which peripheral to use and which physical pins to assign to it.

The DISC1 development board has several onboard peripherals that already consume SPI-capable pins. Before selecting a peripheral, the schematic must be consulted to determine which pins are already spoken for.

Key constraints:
- Bare-metal register-level development — no HAL pin conflict detection
- Must not interfere with onboard DISC1 peripherals
- Pins must be available on the expansion headers for breadboard access

---

## Decision

Use **SPI2 on PB13 (SCK) and PB15 (MOSI)**, with PB12 as manual GPIO chip select.

---

## Options Considered

### Option A — SPI1 (PA5, PA6, PA7)
SPI1 sits on APB2 (up to 84MHz), offering the highest possible clock rate.

**Pros:**
- Highest bus speed of the three SPI peripherals

**Cons:**
- PA5, PA6, PA7 are the default SPI1 pins
- These pins are connected to the LIS3DSH accelerometer on the DISC1 board (confirmed via schematic trace)
- Using SPI1 on these pins would conflict with the onboard accelerometer
- Alternate pin remapping would add complexity

### Option B — SPI2 (PB12, PB13, PB14, PB15)
SPI2 sits on APB1 (up to 42MHz).

**Pros:**
- PB12–PB15 confirmed free via schematic trace — not connected to any onboard DISC1 peripheral
- All four pins available on the P1 expansion header
- 42MHz maximum is far more than needed for MCP4922 (20MHz max SPI)
- AF5 applies uniformly to all four pins — simple configuration

**Cons:**
- Slower maximum clock than SPI1 — irrelevant for this use case

### Option C — SPI3 (PC10, PC11, PC12, PA15)
SPI3 sits on APB1 alongside SPI2.

**Pros:**
- An available option if SPI2 were occupied

**Cons:**
- Not investigated in detail — SPI2 was confirmed free first
- PC and PA pins would need separate schematic verification

---

## Rationale

SPI1 was eliminated immediately by schematic investigation — its default pins are consumed by the onboard accelerometer. SPI2 was selected because schematic tracing confirmed PB12–PB15 are free and routed to expansion headers. The speed advantage of SPI1 is irrelevant for the MCP4922, which requires a modest SPI clock. SPI3 was not evaluated further once SPI2 was confirmed available.

This decision was grounded in hardware evidence (schematic trace) rather than assumption.

---

## Consequences

- All SPI driver code targets SPI2 at base address 0x40003800
- GPIOB must be clocked (RCC_AHB1ENR bit 1) and SPI2 must be clocked (RCC_APB1ENR bit 14)
- PB13 and PB15 require AF5 configuration via GPIOB_AFRH
- PB12 is configured as GPIO output for manual CS control
- If SPI2 is needed for another peripheral in the future, this decision must be revisited

---

## References

- STM32F407 Reference Manual — SPI peripheral, Section 28
- STM32F407 Datasheet — Alternate function mapping table
- DISC1 MB997E Schematic — U4A pin connections
- [SPI Peripheral Deep Dive](../concepts/spi-peripheral.md)
- [ADR-002 — CS Management](adr-002-cs-management.md)
