# Register Map

## SPI2 Bring-Up — All Registers Used

### RCC Registers

| Register | Address | Bit | Field | Value | Purpose |
|---|---|---|---|---|---|
| RCC_AHB1ENR | 0x40023830 | 1 | GPIOBEN | 1 | Enable GPIOB clock |
| RCC_APB1ENR | 0x40023840 | 14 | SPI2EN | 1 | Enable SPI2 clock |

### GPIOB Registers

| Register | Address | Bits | Field | Value | Purpose |
|---|---|---|---|---|---|
| GPIOB_MODER | 0x40020400 | 25:24 | MODER12 | 01 | PB12 = output (CS) |
| GPIOB_MODER | 0x40020400 | 27:26 | MODER13 | 10 | PB13 = alternate function |
| GPIOB_MODER | 0x40020400 | 31:30 | MODER15 | 10 | PB15 = alternate function |
| GPIOB_OTYPER | 0x40020404 | 12 | OT12 | 0 | PB12 push-pull |
| GPIOB_OTYPER | 0x40020404 | 13 | OT13 | 0 | PB13 push-pull |
| GPIOB_OTYPER | 0x40020404 | 15 | OT15 | 0 | PB15 push-pull |
| GPIOB_OSPEEDR | 0x40020408 | 25:24 | OSPEEDR12 | 10 | PB12 high speed |
| GPIOB_OSPEEDR | 0x40020408 | 27:26 | OSPEEDR13 | 10 | PB13 high speed |
| GPIOB_OSPEEDR | 0x40020408 | 31:30 | OSPEEDR15 | 10 | PB15 high speed |
| GPIOB_PUPDR | 0x4002040C | 25:24 | PUPDR12 | 00 | PB12 no pull |
| GPIOB_PUPDR | 0x4002040C | 27:26 | PUPDR13 | 00 | PB13 no pull |
| GPIOB_PUPDR | 0x4002040C | 31:30 | PUPDR15 | 00 | PB15 no pull |
| GPIOB_ODR | 0x40020414 | 12 | ODR12 | 1 | CS idle high |
| GPIOB_AFRH | 0x40020424 | 23:20 | AFRH13 | 0101 | PB13 = AF5 (SPI2_SCK) |
| GPIOB_AFRH | 0x40020424 | 31:28 | AFRH15 | 0101 | PB15 = AF5 (SPI2_MOSI) |

### SPI2 Registers

| Register | Address | Bit(s) | Field | Value | Purpose |
|---|---|---|---|---|---|
| SPI2_CR1 | 0x40003800 | 11 | DFF | 1 | 16-bit data frame |
| SPI2_CR1 | 0x40003800 | 9 | SSM | 1 | Software CS management |
| SPI2_CR1 | 0x40003800 | 8 | SSI | 1 | Internal NSS high |
| SPI2_CR1 | 0x40003800 | 5:3 | BR | 010 | Clock ÷8 (~2MHz) |
| SPI2_CR1 | 0x40003800 | 2 | MSTR | 1 | Master mode |
| SPI2_CR1 | 0x40003800 | 1 | CPOL | 0 | Clock idles low |
| SPI2_CR1 | 0x40003800 | 0 | CPHA | 0 | Sample on rising edge |
| SPI2_CR1 | 0x40003800 | 6 | SPE | 1 | Enable SPI (set last) |
| SPI2_SR | 0x40003808 | 7 | BSY | — | Read: 1=busy |
| SPI2_SR | 0x40003808 | 1 | TXE | — | Read: 1=TX buffer empty |
| SPI2_DR | 0x4000380C | 15:0 | DR | data | Write 16-bit word |

---

## I2C1 Bring-Up — All Registers Used

### RCC Registers

| Register | Address | Bit | Field | Value | Purpose |
|---|---|---|---|---|---|
| RCC_AHB1ENR | 0x40023830 | 1 | GPIOBEN | 1 | Enable GPIOB clock |
| RCC_APB1ENR | 0x40023840 | 21 | I2C1EN | 1 | Enable I2C1 clock |

### GPIOB Registers

| Register | Address | Bits | Field | Value | Purpose |
|---|---|---|---|---|---|
| GPIOB_MODER | 0x40020400 | 13:12 | MODER6 | 10 | PB6 = alternate function (SCL) |
| GPIOB_MODER | 0x40020400 | 15:14 | MODER7 | 10 | PB7 = alternate function (SDA) |
| GPIOB_OTYPER | 0x40020404 | 6 | OT6 | 1 | PB6 open-drain |
| GPIOB_OTYPER | 0x40020404 | 7 | OT7 | 1 | PB7 open-drain |
| GPIOB_OSPEEDR | 0x40020408 | 13:12 | OSPEEDR6 | 01 | PB6 medium speed |
| GPIOB_OSPEEDR | 0x40020408 | 15:14 | OSPEEDR7 | 01 | PB7 medium speed |
| GPIOB_PUPDR | 0x4002040C | 13:12 | PUPDR6 | 01 | PB6 pull-up |
| GPIOB_PUPDR | 0x4002040C | 15:14 | PUPDR7 | 01 | PB7 pull-up |
| GPIOB_AFRL | 0x40020420 | 27:24 | AFRL6 | 0100 | PB6 = AF4 (I2C1_SCL) |
| GPIOB_AFRL | 0x40020420 | 31:28 | AFRL7 | 0100 | PB7 = AF4 (I2C1_SDA) |

### I2C1 Registers

| Register | Address | Bit(s) | Field | Value | Purpose |
|---|---|---|---|---|---|
| I2C1_CR1 | 0x40005400 | 15 | SWRST | 1→0 | Software reset (clear before configure) |
| I2C1_CR1 | 0x40005400 | 0 | PE | 1 | Peripheral enable (set last) |
| I2C1_CR1 | 0x40005400 | 8 | START | 1 | Generate START condition |
| I2C1_CR1 | 0x40005400 | 9 | STOP | 1 | Generate STOP condition |
| I2C1_CR2 | 0x40005404 | 5:0 | FREQ | 16 | APB1 frequency in MHz |
| I2C1_DR | 0x40005410 | 7:0 | DR | data | Byte to transmit |
| I2C1_SR1 | 0x40005414 | 0 | SB | — | Read: 1=START sent |
| I2C1_SR1 | 0x40005414 | 1 | ADDR | — | Read: 1=address phase complete |
| I2C1_SR1 | 0x40005414 | 2 | BTF | — | Read: 1=byte transfer finished |
| I2C1_SR1 | 0x40005414 | 7 | TxE | — | Read: 1=TX register empty |
| I2C1_SR2 | 0x40005418 | 1 | BUSY | — | Read: 1=bus busy |
| I2C1_CCR | 0x4000541C | 11:0 | CCR | 80 | 100 kHz @ 16 MHz: 16M/(2×100k) |
| I2C1_TRISE | 0x40005420 | 5:0 | TRISE | 17 | (1000 ns × 16 MHz) + 1 |

**ADDR flag clear sequence:** read SR1 (in the poll loop), then read SR2 — the pair clears ADDR atomically.

---

## Session 01 Reference Registers

| Register | Address | Purpose |
|---|---|---|
| RCC_AHB1ENR | 0x40023830 | GPIO clock enables |
| GPIOD_MODER | 0x40020C00 | Port D pin mode |
| GPIOD_ODR | 0x40020C14 | Port D output data |
| FPU_CPACR | 0xE000ED88 | FPU enable |
| SCB_VTOR | 0xE000ED08 | Vector table offset |
| SCB_CFSR | 0xE000ED28 | Configurable fault status |
| SCB_HFSR | 0xE000ED2C | Hard fault status |

---

## GPIOD LED Pins (DISC1)

| LED | Pin | MODER bits |
|---|---|---|
| LD4 Green | PD12 | 25:24 |
| LD3 Orange | PD13 | 27:26 |
| LD5 Red | PD14 | 29:28 |
| LD6 Blue | PD15 | 31:30 |
