#ifndef SPI2_H
#define SPI2_H

#include <stdint.h>

/*
 * Minimal polling SPI2 master driver for the MCP4922 DAC (write-only).
 *
 * SPI2 on PB13 (SCK, AF5) / PB15 (MOSI, AF5); PB12 is the chip-select, driven
 * manually as a GPIO output (software CS). 16-bit frames, CPOL=0/CPHA=0, MSB
 * first, BR=/8 (~2 MHz at the default 16 MHz APB1). See firmware/spi-driver.md
 * and decisions/adr-001..003.
 */

/* Configure GPIO + SPI2 and enable the peripheral. Call once at startup. */
void spi2_init(void);

/* Transmit one 16-bit word: CS low, wait TXE, halfword write, wait !BSY, CS high. */
void spi2_write16(uint16_t data);

#endif /* SPI2_H */
