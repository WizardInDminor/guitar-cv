#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

/*
 * Minimal write-only SSD1306 128×64 OLED driver over I2C1 (no frame buffer).
 * Uses page addressing mode. Call i2c1_init() before any function here.
 *
 * Built for I2C address 0x3D (SA0 pin tied high), which is what the bench
 * module straps and what was verified on hardware. If your module ties SA0
 * low, change SSD1306_ADDR in ssd1306.c to 0x3C.
 */

/* Send the full initialization sequence and turn the display on. */
void ssd1306_init(void);

/* Fill all 8 pages (128×64 px) with pattern. 0x00=black, 0xFF=white. */
void ssd1306_fill(uint8_t pattern);

/*
 * Draw a NUL-terminated string in the 5×7 font.
 *   page : text row 0..7 (each row is one page, 8 px tall)
 *   col  : starting column in pixels, 0..127
 * Each glyph occupies 6 columns (5 font + 1 spacing), so a full row holds 21
 * characters. Text is clipped at the right edge — it does not wrap to the next
 * page. Characters outside 0x20..0x7E render as blanks. There is no background
 * erase: call ssd1306_fill(0x00) or overwrite with spaces to clear.
 */
void ssd1306_text(uint8_t page, uint8_t col, const char *s);

#endif /* SSD1306_H */
