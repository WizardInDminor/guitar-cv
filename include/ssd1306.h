/*
 * SSD1306 128x64 OLED driver over I2C2 (see include/i2c.h).
 *
 * Thin hardware glue on top of the I2C driver: an init sequence, a few drawing
 * helpers, and a 5x7 text renderer. The display is organised as 8 pages
 * (rows of 8 vertical pixels) x 128 columns; text is placed on a page boundary.
 *
 * Framebuffer-ready: rendering goes through ssd1306_data(), which streams a byte
 * buffer straight to GDDRAM. A later RAM framebuffer just calls
 * ssd1306_window(0,0,128,8) then ssd1306_data(fb, 1024) to flush — no rework of
 * this layer. See firmware/ssd1306-driver.md.
 */
#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

#define SSD1306_ADDR   0x3C  /* 7-bit; some modules strap to 0x3D */
#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT 64
#define SSD1306_PAGES  (SSD1306_HEIGHT / 8)  /* 8 */

/* Bring up I2C2 and run the SSD1306 init sequence. Call once at startup. */
void ssd1306_init(void);

/* Fill the whole panel with a column pattern (0x00 = off, 0xFF = all on). */
void ssd1306_fill(uint8_t pattern);

/* Clear the panel (all pixels off). */
void ssd1306_clear(void);

/*
 * Draw a NUL-terminated string in the 5x7 font.
 *   page : text row 0..7  (each row is 8 px tall)
 *   col  : starting column 0..127 (pixels)
 * Each glyph is 6 columns wide (5 font + 1 spacing). Text is clipped at the
 * right edge; characters outside 0x20..0x7E render as blanks.
 */
void ssd1306_text(uint8_t page, uint8_t col, const char *s);

/* --- Lower-level primitives (also used by a future framebuffer) --- */

/* Send a single command byte. */
void ssd1306_command(uint8_t cmd);

/* Set the GDDRAM auto-increment window (horizontal addressing mode). */
void ssd1306_window(uint8_t col, uint8_t page, uint8_t width, uint8_t pages);

/* Stream a buffer of pixel/data bytes into the current window. */
void ssd1306_data(const uint8_t *buf, uint32_t len);

#endif /* SSD1306_H */
