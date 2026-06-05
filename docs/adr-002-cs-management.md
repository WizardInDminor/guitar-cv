# ADR-002 — Chip Select Management Strategy

**Date:** 2026-04-10  
**Status:** Accepted  
**Supersedes:** —  
**Superseded by:** —

---

## Context

The STM32 SPI peripheral includes hardware NSS (chip select) management. There are two modes: hardware NSS, where the peripheral drives the CS pin automatically, and software NSS, where the developer controls CS manually via GPIO. The choice has implications for transaction control, code clarity, and correctness with the MCP4922.

The MCP4922 requires CS to be held low for the entire 16-bit transaction and return high after the 16th clock pulse. The DAC latches the output value on the rising edge of CS.

---

## Decision

Use **software CS management (SSM=1, SSI=1)** with PB12 controlled manually as a GPIO output.

---

## Options Considered

### Option A — Hardware NSS (SSM=0)
The STM32 peripheral automatically drives the NSS pin low at the start of a transaction and high at the end.

**Pros:**
- No manual GPIO toggling required
- Automatic timing

**Cons:**
- Hardware NSS behavior has known quirks on STM32 — it can deassert CS between bytes in certain configurations
- In 16-bit DFF mode the behavior is less predictable than in 8-bit mode
- Hardware NSS requires using the designated NSS pin (PB12 as SPI2_NSS), locking that pin to the peripheral
- Harder to control precisely when CS asserts and deasserts relative to data
- More difficult to debug — behavior is implicit rather than explicit

### Option B — Software CS (SSM=1, SSI=1)
CS is a plain GPIO output (PB12). The developer pulls it low before the SPI write and high after the transaction completes.

**Pros:**
- Explicit, readable, debuggable — CS state is always visible in code
- Full control over CS timing relative to data
- No risk of hardware NSS quirks affecting the transaction
- Clean and well-understood — the same pattern used in most embedded SPI drivers
- Consistent with the project philosophy of staying close to the hardware

**Cons:**
- Requires two additional register writes per transaction (CS low, CS high)
- Slightly more code than hardware NSS

---

## Rationale

Software CS was chosen for clarity and reliability. The STM32 hardware NSS system introduces subtle timing behaviors that vary by configuration, and debugging a CS glitch on a new peripheral bring-up adds unnecessary risk. Manual GPIO CS is explicit — the code says exactly what the hardware does. This aligns with the project's bare-metal philosophy of preferring understanding and control over convenience.

The additional code overhead (two register writes) is negligible.

SSM=1 disables hardware NSS management. SSI=1 forces the internal NSS signal high inside the peripheral so it does not believe it has been selected by an external master — a required companion setting when using software CS.

---

## Consequences

- PB12 is configured as GPIO output, not as SPI2_NSS alternate function
- Every SPI transaction includes explicit CS assert/deassert around the SPI_DR write
- The transmit function follows the pattern: CS low → wait TXE → write DR → wait BSY → CS high
- Future peripherals sharing the SPI bus would each require their own GPIO CS pin

---

## References

- STM32F407 Reference Manual — SPI Section, SSM/SSI bit description
- MCP4922 Datasheet — Figure 5.1, CS timing requirements
- [SPI Peripheral Deep Dive](../concepts/spi-peripheral.md)
- [ADR-001 — SPI Peripheral Selection](adr-001-spi-peripheral.md)
