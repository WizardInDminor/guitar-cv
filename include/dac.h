#ifndef DAC_H
#define DAC_H

#include <stdint.h>

#include "spi2.h"

/*
 * MCP4922 CV output, on top of the SPI2 driver. Thin hardware glue: the
 * command-word packing (mcp4922_command) and note->count math (note_to_dac)
 * are pure, host-tested functions; this layer just transmits.
 */

/*
 * Bring up SPI2 for the DAC from the actual APB1 clock. Call once at
 * startup. Applies the DAC's SCK policy (2 MHz bring-up cap, ADR-003;
 * MCP4922 absolute max is 20 MHz).
 */
spi_status_t dac_init(uint32_t pclk1_hz);

/* Write a 12-bit value to a DAC channel (MCP4922_CHANNEL_A / _B). */
void dac_write(uint8_t channel, uint16_t value);

#endif /* DAC_H */
