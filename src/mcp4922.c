#include "mcp4922.h"

uint16_t mcp4922_command(uint8_t channel, uint16_t value)
{
    uint16_t cmd = 0;

    cmd |= (uint16_t)((channel & 0x1u) << 15); /* channel select        */
    cmd |= (uint16_t)(1u << 13);               /* /GA   = 1 -> 1x gain  */
    cmd |= (uint16_t)(1u << 12);               /* /SHDN = 1 -> active   */
    cmd |= (uint16_t)(value & 0x0FFFu);        /* 12-bit data           */

    return cmd;
}
