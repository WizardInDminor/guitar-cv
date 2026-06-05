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
