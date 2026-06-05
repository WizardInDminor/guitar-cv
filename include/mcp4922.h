#ifndef MCP4922_H
#define MCP4922_H

#include <stdint.h>

/* DAC channel selects (bit 15 of the command word). */
#define MCP4922_CHANNEL_A 0u
#define MCP4922_CHANNEL_B 1u

/*
 * Build the 16-bit MCP4922 command word for a write:
 *
 *   bit 15  /A·B  channel select (0 = A, 1 = B)
 *   bit 14  BUF   0 = unbuffered Vref
 *   bit 13  /GA   1 = 1x gain
 *   bit 12  /SHDN 1 = output active (not shut down)
 *   bits 11:0     12-bit data
 *
 * Pure function (no SPI access): the bench session's mcp4922_write() will be
 *   spi2_write16(mcp4922_command(channel, value));
 */
uint16_t mcp4922_command(uint8_t channel, uint16_t value);

#endif /* MCP4922_H */
