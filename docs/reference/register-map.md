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

## I2C2 + SSD1306 Bring-Up — All Registers Used

### RCC Registers

| Register | Address | Bit | Field | Value | Purpose |
|---|---|---|---|---|---|
| RCC_AHB1ENR | 0x40023830 | 1 | GPIOBEN | 1 | Enable GPIOB clock (shared with SPI2) |
| RCC_APB1ENR | 0x40023840 | 22 | I2C2EN | 1 | Enable I2C2 clock |

### GPIOB Registers (PB10 = SCL, PB11 = SDA)

| Register | Address | Bits | Field | Value | Purpose |
|---|---|---|---|---|---|
| GPIOB_MODER | 0x40020400 | 21:20 | MODER10 | 10 | PB10 = alternate function |
| GPIOB_MODER | 0x40020400 | 23:22 | MODER11 | 10 | PB11 = alternate function |
| GPIOB_OTYPER | 0x40020404 | 10 | OT10 | 1 | PB10 open-drain (I2C) |
| GPIOB_OTYPER | 0x40020404 | 11 | OT11 | 1 | PB11 open-drain (I2C) |
| GPIOB_OSPEEDR | 0x40020408 | 21:20 | OSPEEDR10 | 10 | PB10 high speed |
| GPIOB_OSPEEDR | 0x40020408 | 23:22 | OSPEEDR11 | 10 | PB11 high speed |
| GPIOB_PUPDR | 0x4002040C | 21:20 | PUPDR10 | 01 | PB10 internal pull-up |
| GPIOB_PUPDR | 0x4002040C | 23:22 | PUPDR11 | 01 | PB11 internal pull-up |
| GPIOB_AFRH | 0x40020424 | 11:8 | AFRH10 | 0100 | PB10 = AF4 (I2C2_SCL) |
| GPIOB_AFRH | 0x40020424 | 15:12 | AFRH11 | 0100 | PB11 = AF4 (I2C2_SDA) |

### I2C2 Registers

| Register | Address | Bit(s) | Field | Value | Purpose |
|---|---|---|---|---|---|
| I2C2_CR1 | 0x40005800 | 0 | PE | 1 | Peripheral enable (set last) |
| I2C2_CR1 | 0x40005800 | 8 | START | 1 | Generate START |
| I2C2_CR1 | 0x40005800 | 9 | STOP | 1 | Generate STOP |
| I2C2_CR1 | 0x40005800 | 15 | SWRST | 1→0 | Reset peripheral state machine |
| I2C2_CR2 | 0x40005804 | 5:0 | FREQ | 16 | APB1 clock in MHz |
| I2C2_DR | 0x40005810 | 7:0 | DR | data | Address / data byte |
| I2C2_SR1 | 0x40005814 | 0 | SB | — | Read: 1=START generated |
| I2C2_SR1 | 0x40005814 | 1 | ADDR | — | Read: 1=address ACKed |
| I2C2_SR1 | 0x40005814 | 2 | BTF | — | Read: 1=byte transfer finished |
| I2C2_SR1 | 0x40005814 | 7 | TXE | — | Read: 1=data register empty |
| I2C2_SR1 | 0x40005814 | 10 | AF | — | Read: 1=NACK |
| I2C2_SR2 | 0x40005818 | 1 | BUSY | — | Read: 1=bus busy |
| I2C2_CCR | 0x4000581C | 11:0 | CCR | 80 | Standard mode 100 kHz (F/S=0) |
| I2C2_TRISE | 0x40005820 | 5:0 | TRISE | 17 | Max SCL rise time |

### SSD1306 Control / Command Bytes

| Byte | Meaning |
|---|---|
| 0x3C | I2C slave address (7-bit) |
| 0x00 | Control byte: command bytes follow |
| 0x40 | Control byte: data (GDDRAM) bytes follow |
| 0x8D 0x14 | Charge pump enable (required for 3.3 V supply) |
| 0x20 0x00 | Horizontal addressing mode |
| 0xAF / 0xAE | Display on / off |

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
