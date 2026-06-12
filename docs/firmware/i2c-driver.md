# I2C Driver

## Overview

The I2C driver provides a minimal polling-based write-only interface to the STM32F407 I2C1 peripheral. It is used exclusively to drive the SSD1306 OLED display.

See also: [SSD1306 Wiring](../hardware/wiring-ssd1306.md), [Register Map](../reference/register-map.md)

---

## Pin Assignment

| Signal | Pin | Mode | AF |
|---|---|---|---|
| I2C1_SCL | PB6 | Alternate Function, open-drain | AF4 |
| I2C1_SDA | PB7 | Alternate Function, open-drain | AF4 |

Open-drain is mandatory for I2C — it allows the wired-AND bus topology where any device can pull the line low without a bus conflict.

!!! note "Shared SCL pin"
    The onboard CS43L22 audio codec also uses I2C1_SCL on PB6. This is not a conflict in practice: the codec is never initialized in our firmware and its SDA is on PB9, not PB7. For the final PCB (F405), pin assignments should be reviewed.

---

## I2C Configuration

| Parameter | Value | Reasoning |
|---|---|---|
| Mode | Master | MCU drives the bus |
| Speed | Standard (100 kHz) | Conservative for breadboard bring-up |
| APB1 clock | 16 MHz | Default HSI, no PLL |
| CR2.FREQ | 16 | APB1 frequency in MHz (required by peripheral) |
| CCR | 80 | fAPB1 / (2 × fI2C) = 16M / 200k |
| TRISE | 17 | (1000 ns × 16 MHz) + 1 |
| Pull-ups | Internal | Bring-up only — replace with 4.7 kΩ external resistors on the final board |

---

## Initialization Sequence

```c
void i2c1_init(void)
{
    // 1. Enable GPIOB and I2C1 clocks
    RCC_AHB1ENR |= (1u << 1);   // GPIOBEN
    RCC_APB1ENR |= (1u << 21);  // I2C1EN

    // 2. PB6/PB7 — alternate function (10), open-drain (1), medium speed (01),
    //    internal pull-up (01), AF4 in AFRL
    GPIOB_MODER  = AF on pins 6 and 7
    GPIOB_OTYPER = open-drain on 6 and 7
    GPIOB_OSPEEDR = medium speed on 6 and 7
    GPIOB_PUPDR  = pull-up on 6 and 7
    GPIOB_AFRL   = AF4 on pins 6 and 7

    // 3. Software reset (PE must be 0 to write CCR/TRISE)
    I2C1_CR1 = SWRST
    I2C1_CR1 = 0

    // 4. Timing
    I2C1_CR2   = 16   // FREQ field: APB1 in MHz
    I2C1_CCR   = 80
    I2C1_TRISE = 17

    // 5. Enable (set PE last)
    I2C1_CR1 = PE
}
```

---

## Write Function

The STM32F407 I2C state machine is event-driven: each step waits on a status flag in SR1 or SR2.

```c
void i2c1_write(uint8_t addr7, const uint8_t *buf, uint16_t len)
{
    // Wait for bus free (SR2.BUSY=0)
    while (SR2 & BUSY) { }

    // Assert START; wait SB=1
    CR1 |= START
    while (!(SR1 & SB)) { }

    // Send address + write bit (0); clears SB
    DR = addr7 << 1
    while (!(SR1 & ADDR)) { }
    (void)SR2                   // clear ADDR: read SR1 then SR2

    // Send bytes; each waits TxE=1
    for each byte:
        while (!(SR1 & TxE)) { }
        DR = byte

    // Wait for shift register to empty, then STOP
    while (!(SR1 & BTF)) { }
    CR1 |= STOP
}
```

!!! warning "ADDR clear sequence"
    The ADDR flag is cleared by a specific two-step read: first read SR1 (done in the poll loop), then read SR2. Reading only SR1 is not sufficient.

---

## SSD1306 Framing Over I2C

The SSD1306 uses the first byte after the address as a **control byte**:

| Control byte | Meaning |
|---|---|
| `0x00` | All following bytes in this transfer are **commands** |
| `0x40` | All following bytes in this transfer are **display data** |

This means a complete "write one command" transaction is:

```
START → 0x78 (addr+W) → 0x00 (control) → 0xXX (command) → STOP
```

And a display data burst is:

```
START → 0x78 → 0x40 (control) → [128 data bytes] → STOP
```

The entire SSD1306 init sequence can be sent in one transaction by using control byte `0x00` followed by all command bytes — no per-command START/STOP overhead.

---

## Verification

After wiring the SSD1306 and flashing, check with a logic analyzer or scope on PB6/PB7:

1. SDA/SCL idle high (pull-ups working)
2. START condition: SDA falls while SCL is high
3. Address byte: `0x78` (0x3C shifted left + W=0), ACK from SSD1306
4. Control byte: `0x00`, data bytes, ACK per byte
5. STOP condition: SDA rises while SCL is high
6. OLED shows solid white (`ssd1306_fill(0xFF)`)
