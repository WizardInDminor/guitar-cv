#include "dac.h"
#include "mcp4922.h"
#include "spi2.h"

void dac_init(void)
{
    spi2_init();
}

void dac_write(uint8_t channel, uint16_t value)
{
    spi2_write16(mcp4922_command(channel, value));
}
