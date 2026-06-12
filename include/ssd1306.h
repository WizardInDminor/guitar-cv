#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

/*
 * Minimal write-only SSD1306 128×64 OLED driver over I2C1 (no frame buffer).
 * Uses page addressing mode. Call i2c1_init() before any function here.
 *
 * Default I2C address 0x3C (SA0 pin tied low). If your module has SA0 high,
 * change SSD1306_ADDR in ssd1306.c to 0x3D.
 */

/* Send the full initialization sequence and turn the display on. */
void ssd1306_init(void);

/* Fill all 8 pages (128×64 px) with pattern. 0x00=black, 0xFF=white. */
void ssd1306_fill(uint8_t pattern);

#endif /* SSD1306_H */
