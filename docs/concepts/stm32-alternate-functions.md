# STM32 Alternate Function System

## The Core Problem It Solves

The STM32F407VGT6 has 140 GPIO pins but far more than 140 possible peripheral functions. The solution is that most pins can serve multiple purposes, selected at runtime through configuration registers. Every GPIO pin has up to 16 possible alternate functions, numbered AF0 through AF15.

This is the concept that most commonly trips up developers coming from PIC or AVR, where peripherals map to pins in a more direct way.

---

## The Four Layers

For every pin used as a peripheral function, four things must be configured:

### Layer 1 — RCC Clock Enable
The GPIO port clock must be enabled before any of its registers can be written. This applies to plain GPIO too, but is easy to forget for peripheral pins.

```c
// Enable GPIOB clock
*(volatile uint32_t *)0x40023830 |= (1 << 1);  // RCC_AHB1ENR, bit 1
```

### Layer 2 — MODER (Mode Register)
Each pin has a 2-bit field selecting one of four modes:

| Value | Mode |
|---|---|
| 00 | Input |
| 01 | Output |
| 10 | Alternate Function |
| 11 | Analog |

For SPI pins, set MODER to `0b10` — alternate function mode.

```c
// PB13 → alternate function (bits 27:26)
*(volatile uint32_t *)0x40020400 &= ~(0x3 << 26);
*(volatile uint32_t *)0x40020400 |=  (0x2 << 26);
```

### Layer 3 — AFRL / AFRH (Alternate Function Registers)
The actual mux selection. Each pin gets a 4-bit field selecting AF0–AF15.

- **AFRL** covers pins 0–7
- **AFRH** covers pins 8–15

The AF number for a given peripheral is found in the STM32F407 device datasheet (not the reference manual) in the alternate function mapping table.

```c
// PB13 → AF5 (SPI2_SCK), bits 23:20 of AFRH
*(volatile uint32_t *)0x40020424 &= ~(0xF << 20);
*(volatile uint32_t *)0x40020424 |=  (0x5 << 20);
```

### Layer 4 — Speed, Type, Pull
Output speed (OSPEEDR), output type (OTYPER), and pull-up/pull-down (PUPDR).

For SPI signals:

| Register | Setting | Value |
|---|---|---|
| OTYPER | Push-pull | 0 |
| OSPEEDR | High speed | 0b10 |
| PUPDR | No pull | 0b00 |

---

## The Key Mental Model

On PIC, peripherals own their pins. On STM32, **GPIO owns all pins** and you borrow them to peripherals through this alternate function system. That inversion is the source of most STM32 bring-up confusion.

Once internalized it becomes mechanical — but it must be deliberate every time.

---

## SPI2 Alternate Function Map

For this project, SPI2 uses AF5 on all relevant pins:

| Pin | AF5 Function | Used |
|---|---|---|
| PB12 | SPI2_NSS | No — used as manual GPIO CS |
| PB13 | SPI2_SCK | Yes |
| PB14 | SPI2_MISO | No — MCP4922 is write-only |
| PB15 | SPI2_MOSI | Yes |

---

## Configuration Order

Always follow this order:

1. Enable GPIO clock (RCC_AHB1ENR)
2. Enable peripheral clock (RCC_APB1ENR for SPI2)
3. Set MODER to alternate function mode
4. Set AFRL/AFRH to correct AF number
5. Set OSPEEDR, OTYPER, PUPDR
6. Configure the peripheral registers
7. Enable the peripheral

Never write peripheral registers before enabling its clock — it is a silent no-op.
