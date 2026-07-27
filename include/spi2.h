#ifndef SPI2_H
#define SPI2_H

#include <stdint.h>

/*
 * Minimal polling SPI2 master driver for the MCP4922 DAC (write-only).
 *
 * SPI2 on PB13 (SCK, AF5) / PB15 (MOSI, AF5); PB12 is the chip-select, driven
 * manually as a GPIO output (software CS). 16-bit frames, CPOL=0/CPHA=0, MSB
 * first. The baud divider is computed from the supplied APB1 clock: the
 * fastest divider whose SCK does not exceed max_sck_hz (2 MHz bring-up policy
 * per ADR-003). See firmware/spi-driver.md and decisions/adr-001..003.
 */

typedef enum {
    SPI_OK = 0,
    SPI_ERR_INVALID_CONFIG,  /* no legal divider reaches max_sck_hz */
} spi_status_t;

/* Configure GPIO + SPI2 from the actual APB1 clock. Call once at startup. */
spi_status_t spi2_init(uint32_t pclk1_hz, uint32_t max_sck_hz);

/* Transmit one 16-bit word: CS low, wait TXE, halfword write, wait !BSY, CS high. */
void spi2_write16(uint16_t data);

/*
 * Pure baud-math helpers — no hardware access, unit-tested on the host
 * (test/test_clock.c).
 *
 * BR[2:0] selects SCK = pclk / 2^(BR+1), BR = 0..7 (/2../256). Returns the
 * smallest BR (fastest SCK) with SCK <= max_hz, or 8 if even /256 is too
 * fast (invalid config).
 */
static inline uint32_t spi_br_for_max_hz(uint32_t pclk_hz, uint32_t max_hz)
{
    for (uint32_t br = 0; br <= 7u; br++) {
        if ((pclk_hz >> (br + 1u)) <= max_hz) {
            return br;
        }
    }
    return 8u;
}

/* Resulting SCK for a given BR value. */
static inline uint32_t spi_sck_hz(uint32_t pclk_hz, uint32_t br)
{
    return pclk_hz >> (br + 1u);
}

#endif /* SPI2_H */
