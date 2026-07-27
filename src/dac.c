#include "dac.h"
#include "mcp4922.h"
#include "spi2.h"

/*
 * Bring-up SCK cap per ADR-003: 2 MHz is robust on a breadboard and well
 * under the MCP4922's 20 MHz limit. The driver picks the fastest divider
 * that stays at or below this.
 */
#define DAC_MAX_SCK_HZ 2000000u

spi_status_t dac_init(uint32_t pclk1_hz)
{
    return spi2_init(pclk1_hz, DAC_MAX_SCK_HZ);
}

void dac_write(uint8_t channel, uint16_t value)
{
    spi2_write16(mcp4922_command(channel, value));
}
