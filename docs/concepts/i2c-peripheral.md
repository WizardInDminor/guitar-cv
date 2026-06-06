# I2C Peripheral Deep Dive

How the STM32F407 I2C peripheral works and how this project drives it for the SSD1306 OLED.
Companion to the [SPI Peripheral Deep Dive](spi-peripheral.md); the
[STM32 Alternate Function System](stm32-alternate-functions.md) covers the GPIO plumbing
that applies here too.

---

## Why I2C is different from SPI

SPI is push-pull and full-duplex with a dedicated chip-select. I2C is a **two-wire,
open-drain, addressable** bus:

- **SCL** (clock) and **SDA** (data) are both **open-drain** — devices can only pull the
  line low; **pull-up resistors** return it high. This is what lets multiple devices and
  bidirectional data share two wires without contention.
- Every transaction is **addressed**: the master sends a 7-bit address + a R/W bit, and the
  addressed slave ACKs each byte by pulling SDA low on the 9th clock.
- There is no separate select line — START and STOP conditions frame each transaction.

For this project the OLED is **write-only** from our side (we never read it back), so we
only use the master-transmit path.

---

## The bus we use

| Parameter | Value | Why |
|---|---|---|
| Peripheral | I2C2 | I2C1 pins taken by onboard codec — see [ADR-004](../decisions/adr-004-i2c-peripheral.md) |
| SCL | PB10, AF4 | Free, on the P1 header |
| SDA | PB11, AF4 | Free, on the P1 header |
| Output type | Open-drain | Mandatory for I2C |
| Pull-ups | Internal (~40 kΩ) + module 4.7 kΩ | Idle-high lines |
| Speed | Standard mode, 100 kHz | Safe bring-up rate (SSD1306 supports 400 kHz) |
| APB1 clock | 16 MHz (HSI, no PLL yet) | Sets the timing register values |

---

## The four GPIO layers (I2C variant)

Same four-layer model as any alternate-function pin, but with I2C-specific choices:

| Layer | Register | SPI value | **I2C value** |
|---|---|---|---|
| 1 — Clock enable | RCC | GPIOB + SPI2 | GPIOB + I2C2 (APB1 bit 22) |
| 2 — Mode | MODER | 10 (AF) | 10 (AF) |
| 3 — Alternate fn | AFRH | AF5 | **AF4** |
| 4a — Output type | OTYPER | 0 (push-pull) | **1 (open-drain)** |
| 4b — Pull | PUPDR | 00 (none) | **01 (pull-up)** |
| 4c — Speed | OSPEEDR | 10 (high) | 10 (high) |

The two highlighted differences — **open-drain** and **pull-up** — are the whole reason an
I2C pin is configured differently from an SPI pin.

---

## Timing registers

Standard-mode timing is derived from the APB1 clock (currently 16 MHz):

```
FREQ  = APB1 in MHz            = 16
CCR   = APB1 / (2 * F_scl)     = 16e6 / (2 * 100e3) = 80
TRISE = FREQ + 1               = 17     (1000 ns max rise time, standard mode)
```

| Register | Field | Value | Meaning |
|---|---|---|---|
| I2C2_CR2 | FREQ[5:0] | 16 | Peripheral clock in MHz (for timing) |
| I2C2_CCR | CCR[11:0] | 80 | High/low period count; F/S=0 → standard mode |
| I2C2_TRISE | TRISE[5:0] | 17 | Max SCL rise time |

!!! warning "These values are tied to 16 MHz APB1"
    When a later session enables the PLL and raises APB1, FREQ/CCR/TRISE must be
    recomputed — exactly the same caveat as the SPI baud-rate divisor in
    [ADR-003](../decisions/adr-003-spi-clock-rate.md).

---

## A master-transmit transaction, step by step

This is the polling sequence the driver follows (`src/i2c.c`):

1. **Wait** for the bus to be free (`SR2.BUSY = 0`).
2. **START** — set `CR1.START`; wait for `SR1.SB` (start generated).
3. **Address** — write `(addr << 1) | 0` to `DR`; wait for `SR1.ADDR` (address ACKed).
4. **Clear ADDR** — read `SR1` then `SR2` (the documented clearing sequence).
5. **Data** — for each byte: wait `SR1.TXE`, write `DR`.
6. **Finish** — wait `SR1.BTF` (last byte shifted out).
7. **STOP** — set `CR1.STOP`.

Every wait is bounded by a timeout so a missing or miswired device leaves the MCU running
(the heartbeat LED keeps blinking) instead of hanging on a flag that never sets — a
deliberate departure from the SPI driver's unbounded spins, justified because an
unacknowledged I2C address would otherwise lock the bus forever.

---

## Key status flags

| Register | Bit | Field | Meaning |
|---|---|---|---|
| SR1 | 0 | SB | START condition generated |
| SR1 | 1 | ADDR | Address sent and ACKed |
| SR1 | 2 | BTF | Byte transfer finished |
| SR1 | 7 | TXE | Data register empty |
| SR1 | 10 | AF | Acknowledge failure (NACK) |
| SR2 | 1 | BUSY | Bus busy |

See the [Register Map](../reference/register-map.md) for full addresses and the
[SSD1306 OLED Driver](../firmware/ssd1306-driver.md) for how the command/data framing sits
on top of this transaction.
