#ifndef DAC_H
#define DAC_H

#include <stdint.h>

/*
 * MCP4922 CV output, on top of the SPI2 driver. Thin hardware glue: the
 * command-word packing (mcp4922_command) and note->count math (note_to_dac)
 * are pure, host-tested functions; this layer just transmits.
 */

/* Bring up SPI2 for the DAC. Call once at startup. */
void dac_init(void);

/* Write a 12-bit value to a DAC channel (MCP4922_CHANNEL_A / _B). */
void dac_write(uint8_t channel, uint16_t value);

#endif /* DAC_H */
