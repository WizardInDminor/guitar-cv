#ifndef I2C1_H
#define I2C1_H

#include <stdint.h>

/*
 * Minimal polling I2C1 master driver (write-only).
 *
 * I2C1 on PB6 (SCL, AF4) / PB7 (SDA, AF4); open-drain, internal pull-ups.
 * Standard mode (100 kHz) at 16 MHz APB1. Used to drive the SSD1306 OLED.
 * See docs/firmware/i2c-driver.md and docs/reference/register-map.md.
 *
 * Note: external 4.7 kΩ pull-ups are preferred over the weak internal ones;
 * use internal pull-ups only for initial bring-up on a short wire.
 */

/* Configure GPIO + I2C1 and enable the peripheral. Call once at startup. */
void i2c1_init(void);

/*
 * Transmit len bytes from buf to the 7-bit address addr7.
 * Generates: START → addr+W → buf[0..len-1] → STOP.
 */
void i2c1_write(uint8_t addr7, const uint8_t *buf, uint16_t len);

#endif /* I2C1_H */
