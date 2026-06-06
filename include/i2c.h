/*
 * Minimal polling I2C2 master driver for the SSD1306 OLED.
 *
 * I2C2 on PB10 (SCL, AF4) / PB11 (SDA, AF4), open-drain with internal pull-ups,
 * standard mode 100 kHz at the default 16 MHz APB1. Write-only master transmit
 * (the OLED is never read back). See firmware/ssd1306-driver.md,
 * concepts/i2c-peripheral.md and decisions/adr-004-i2c-peripheral.
 *
 * Two layers are exposed:
 *   - a convenience whole-transaction write (i2c2_write), and
 *   - streaming primitives (start / write_byte / stop) so a future framebuffer
 *     flush can push 1 KB of pixel data with no temporary buffer.
 */
#ifndef I2C_H
#define I2C_H

#include <stdint.h>

/* Bring up I2C2 (clocks, PB10/PB11 AF4, timing). Call once at startup. */
void i2c2_init(void);

/*
 * Blocking master-transmit transaction: START, addr (write), len bytes, STOP.
 * addr7 is the 7-bit slave address. Returns 0 on success, non-zero on
 * timeout/NACK (best-effort — a missing device must not lock the MCU).
 */
int i2c2_write(uint8_t addr7, const uint8_t *data, uint32_t len);

/* --- Streaming primitives (compose your own transaction) --- */

/* START + 7-bit address with the R/W bit clear (write). Returns 0 on success. */
int i2c2_start(uint8_t addr7);
/* Push one data byte (waits for the TX register to drain). 0 on success. */
int i2c2_write_byte(uint8_t b);
/* Generate STOP and release the bus. */
void i2c2_stop(void);

#endif /* I2C_H */
