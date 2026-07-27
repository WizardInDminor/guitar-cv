# SPI Driver

## Overview

The SPI driver provides a minimal polling-based interface to the STM32F407 SPI2 peripheral. It is used exclusively to drive the MCP4922 DAC for CV output.

See also: [SPI Peripheral Deep Dive](../concepts/spi-peripheral.md), [ADR-001](../decisions/adr-001-spi-peripheral.md), [ADR-002](../decisions/adr-002-cs-management.md), [ADR-003](../decisions/adr-003-spi-clock-rate.md)

---

## Pin Assignment

| Signal | Pin | Mode | AF |
|---|---|---|---|
| SPI2_SCK | PB13 | Alternate Function | AF5 |
| SPI2_MOSI | PB15 | Alternate Function | AF5 |
| CS (MCP4922) | PB12 | GPIO Output | — |
| SPI2_MISO | PB14 | Not connected | — |

CS is managed manually as a GPIO output. MISO is unused — the MCP4922 is write-only.

---

## SPI Configuration

| Parameter | Value | Reasoning |
|---|---|---|
| Mode | Master | MCU drives the bus |
| Data frame | 16-bit | MCP4922 requires one 16-bit word per transaction |
| Clock polarity | CPOL=0 | Clock idles low (per MCP4922 datasheet Fig 5.1) |
| Clock phase | CPHA=0 | Sample on first (rising) edge |
| Bit order | MSB first | Required by MCP4922 |
| Baud rate divisor | Computed from PCLK1 | Fastest divider with SCK ≤ 2 MHz cap (ADR-003): ÷8 = 2 MHz @ 16 MHz APB1; ÷32 = 1.3125 MHz @ 42 MHz APB1 (post-PLL, see [clock layer](clock.md)) |
| CS management | Software (SSM=1) | Manual GPIO control for clarity and reliability |

---

## Initialization Sequence

```c
void spi2_init(void) {
    // 1. Enable GPIOB clock
    *(volatile uint32_t *)0x40023830 |= (1 << 1);

    // 2. Enable SPI2 clock
    *(volatile uint32_t *)0x40023840 |= (1 << 14);

    // 3. Configure PB12 as output (CS), PB13 and PB15 as AF
    *(volatile uint32_t *)0x40020400 &= ~((0x3 << 24) |
                                          (0x3 << 26) |
                                          (0x3 << 30));
    *(volatile uint32_t *)0x40020400 |=  ((0x1 << 24) |  // PB12 output
                                          (0x2 << 26) |  // PB13 AF
                                          (0x2 << 30));  // PB15 AF

    // 4. Set AF5 on PB13 and PB15 (AFRH)
    *(volatile uint32_t *)0x40020424 &= ~((0xF << 20) | (0xF << 28));
    *(volatile uint32_t *)0x40020424 |=  ((0x5 << 20) | (0x5 << 28));

    // 5. Set high speed, push-pull, no pull on PB12/13/15
    *(volatile uint32_t *)0x40020408 &= ~((0x3 << 24) | (0x3 << 26) | (0x3 << 30));
    *(volatile uint32_t *)0x40020408 |=  ((0x2 << 24) | (0x2 << 26) | (0x2 << 30));
    *(volatile uint32_t *)0x40020404 &= ~((1 << 12) | (1 << 13) | (1 << 15));
    *(volatile uint32_t *)0x4002040C &= ~((0x3 << 24) | (0x3 << 26) | (0x3 << 30));

    // 6. Drive CS high before enabling SPI
    *(volatile uint32_t *)0x40020414 |= (1 << 12);

    // 7. Configure SPI2_CR1 (do not enable yet)
    uint32_t cr1 = 0;
    cr1 |= (1 << 11);  // DFF: 16-bit frame
    cr1 |= (1 << 9);   // SSM: software CS management
    cr1 |= (1 << 8);   // SSI: internal NSS high
    cr1 |= (2 << 3);   // BR: divide by 8
    cr1 |= (1 << 2);   // MSTR: master mode
    // CPOL=0, CPHA=0, LSBFIRST=0 — all zero
    *(volatile uint32_t *)0x40003800 = cr1;

    // 8. Enable SPI2
    *(volatile uint32_t *)0x40003800 |= (1 << 6);
}
```

---

## Transmit Function

```c
void spi2_write16(uint16_t data) {
    // Assert CS
    *(volatile uint32_t *)0x40020414 &= ~(1 << 12);

    // Wait for TXE — transmit buffer empty
    while (!(*(volatile uint32_t *)0x40003808 & (1 << 1)));

    // Write 16-bit word — halfword write required in DFF=1 mode
    *(volatile uint16_t *)0x4000380C = data;

    // Wait for BSY cleared — transaction complete
    while (*(volatile uint32_t *)0x40003808 & (1 << 7));

    // Deassert CS
    *(volatile uint32_t *)0x40020414 |= (1 << 12);
}
```

!!! warning "Halfword Write"
    SPI_DR must be written as `uint16_t` (halfword) when DFF=1. Writing as `uint32_t` will not produce the expected 16-bit transaction.

---

## Verification

After bring-up, verify on oscilloscope:

- CS goes low before SCK activity
- 16 clock pulses per transaction
- SCK idles low between transactions (CPOL=0)
- Data stable before rising edge of SCK (CPHA=0)
- CS returns high after 16th clock pulse
