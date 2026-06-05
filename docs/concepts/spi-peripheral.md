# SPI Peripheral Deep Dive

## What SPI Is

SPI (Serial Peripheral Interface) is a synchronous serial communication protocol. It uses four signals:

| Signal | Direction | Purpose |
|---|---|---|
| SCK | Master → Slave | Clock |
| MOSI | Master → Slave | Data out from master |
| MISO | Slave → Master | Data in to master |
| CS/NSS | Master → Slave | Chip select (active low) |

The master generates the clock. Data is shifted out on one edge and sampled on the other. CS going low selects the target device; CS going high ends the transaction.

---

## Clock Polarity and Phase (CPOL / CPHA)

These two bits define the clock behavior and are the most common source of SPI configuration errors.

**CPOL — Clock Polarity**
Defines the idle state of the clock line between transactions.

| CPOL | Clock idle state |
|---|---|
| 0 | Low |
| 1 | High |

**CPHA — Clock Phase**
Defines which edge data is sampled on.

| CPHA | Sample edge (when CPOL=0) |
|---|---|
| 0 | Rising edge (first edge) |
| 1 | Falling edge (second edge) |

Together they define four SPI modes:

| Mode | CPOL | CPHA | Clock idle | Sample on |
|---|---|---|---|---|
| 0 | 0 | 0 | Low | Rising |
| 1 | 0 | 1 | Low | Falling |
| 2 | 1 | 0 | High | Falling |
| 3 | 1 | 1 | High | Rising |

**The MCP4922 uses Mode 0** — clock idles low, data sampled on rising edge. This was determined directly from the timing diagram in the MCP4922 datasheet (Figure 5.1).

---

## STM32 SPI Registers

### SPI_CR1 — Control Register 1

The primary configuration register. Key bits:

| Bit | Name | Description |
|---|---|---|
| 15 | BIDIMODE | 0 = 2-line unidirectional |
| 11 | DFF | 0 = 8-bit frame, 1 = 16-bit frame |
| 10 | RXONLY | 0 = full duplex |
| 9 | SSM | 1 = software slave management |
| 8 | SSI | 1 = internal NSS high |
| 7 | LSBFIRST | 0 = MSB first |
| 6 | SPE | 1 = SPI enable |
| 5:3 | BR[2:0] | Baud rate divisor |
| 2 | MSTR | 1 = master mode |
| 1 | CPOL | Clock polarity |
| 0 | CPHA | Clock phase |

### Baud Rate Divisor (BR[2:0])

Divides the peripheral bus clock (APB1 for SPI2):

| BR[2:0] | Divisor | At 16MHz APB1 |
|---|---|---|
| 000 | ÷2 | 8 MHz |
| 001 | ÷4 | 4 MHz |
| 010 | ÷8 | 2 MHz |
| 011 | ÷16 | 1 MHz |

### SSM and SSI — Software Slave Management

STM32 has hardware NSS management built in. With SSM=0, the hardware tries to manage the CS pin automatically, which causes problems for manual CS control. Setting SSM=1 disables hardware NSS management. SSI=1 then forces the internal NSS signal high so the peripheral doesn't think it's been selected by an external master.

Together: **SSM=1, SSI=1 = software controls CS, hardware stays out of the way.**

### SPI_SR — Status Register

Read-only. Key bits:

| Bit | Name | Meaning |
|---|---|---|
| 7 | BSY | 1 = peripheral busy, transaction in progress |
| 1 | TXE | 1 = transmit buffer empty, safe to write new data |

### SPI_DR — Data Register

Write data here to transmit. In 16-bit mode (DFF=1), write as `uint16_t` — a 32-bit write will not produce the correct transaction.

---

## Why 16-bit Mode for MCP4922

The MCP4922 requires exactly one 16-bit word per transaction. Options:

- **8-bit mode**: requires two writes to SPI_DR, with CS held low manually across both — more complex
- **16-bit mode (DFF=1)**: one write sends all 16 bits — simpler, cleaner, no risk of CS glitching between bytes

16-bit mode is the correct choice.

---

## The STM32 SPI Peripheral Bus

SPI1 sits on APB2 (up to 84MHz). SPI2 and SPI3 sit on APB1 (up to 42MHz). Before PLL configuration, APB1 runs at the default 16MHz HSI clock. At ÷8, SPI2 runs at 2MHz — well within MCP4922's 20MHz maximum.
