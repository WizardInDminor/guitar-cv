# ADR-003 — SPI Clock Rate for Bring-Up

**Date:** 2026-04-10  
**Status:** Accepted  
**Supersedes:** —  
**Superseded by:** —

---

## Context

The STM32 SPI baud rate is set by the BR[2:0] bits in SPI_CR1, which divide the peripheral bus clock. SPI2 sits on APB1. Before PLL configuration, APB1 runs at the default 16MHz HSI clock.

The MCP4922 supports a maximum SPI clock of 20MHz. In theory, the fastest bring-up clock would be APB1 ÷ 2 = 8MHz. However, bring-up is happening on a breadboard with jumper wires, which introduces parasitic capacitance and inductance that degrade signal integrity at higher clock rates.

---

## Decision

Use **BR[2:0] = 010 (÷8)**, giving a 2MHz SPI clock during bring-up.

---

## Options Considered

### Option A — ÷2 (8MHz)
Maximum practical clock rate on APB1 at default HSI.

**Pros:**
- Fastest possible transactions

**Cons:**
- Breadboard wiring introduces signal integrity risk at 8MHz
- If bring-up fails, signal quality becomes a debugging variable alongside firmware correctness
- No benefit for bring-up — the MCP4922 doesn't care how fast we talk to it during initial verification

### Option B — ÷8 (2MHz)
Conservative clock rate well within MCP4922 limits.

**Pros:**
- Clean signal edges on breadboard jumper wires at 2MHz
- Eliminates signal integrity as a debugging variable during bring-up
- Still fast enough to verify correct SPI behavior on oscilloscope
- Well within MCP4922's 20MHz maximum
- Easy to increase later once bring-up is proven

**Cons:**
- Slower than necessary for final implementation

### Option C — ÷16 (1MHz)
Very conservative.

**Pros:**
- Maximum signal integrity margin

**Cons:**
- Unnecessarily slow — 2MHz already provides ample margin on a breadboard

---

## Rationale

During bring-up the goal is to verify firmware correctness, not to maximize performance. A conservative clock rate eliminates signal integrity as a variable. If the SPI transaction does not work at 2MHz, the problem is in the firmware or wiring — not the clock rate. This simplifies debugging significantly.

The principle applied here: **isolate one variable at a time**. Clock rate is not the variable under test during bring-up.

Clock rate can be increased in a later session once the SPI driver is proven correct and the system clock is configured via PLL.

---

## Consequences

- BR[2:0] = 010 is set in SPI2_CR1 during initialization
- At default 16MHz APB1: SPI clock = 2MHz
- After PLL configuration to 42MHz APB1: same BR setting yields 5.25MHz — still acceptable, revisit if higher throughput is needed
- This setting is appropriate for development board use; PCB implementation may warrant revisiting

---

## References

- MCP4922 Datasheet — Section 1.0, Electrical Characteristics, maximum SPI clock
- STM32F407 Reference Manual — SPI_CR1 BR[2:0] description
- [SPI Peripheral Deep Dive](../concepts/spi-peripheral.md)
- [ADR-001 — SPI Peripheral Selection](adr-001-spi-peripheral.md)
