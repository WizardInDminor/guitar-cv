/*
 * SSD1306 128x64 OLED driver — see include/ssd1306.h.
 *
 * Uses the I2C2 driver's streaming primitives so pixel data of any length
 * (a single glyph column or a full 1 KB frame) goes out in one transaction with
 * no temporary buffer. Command framing follows the SSD1306 datasheet: the first
 * byte after the slave address is a control byte — 0x00 = "command bytes follow",
 * 0x40 = "data (GDDRAM) bytes follow".
 */
#include "ssd1306.h"
#include "i2c.h"
#include "font5x7.h"

#define CTRL_CMD  0x00
#define CTRL_DATA 0x40

void ssd1306_command(uint8_t cmd)
{
    uint8_t buf[2] = { CTRL_CMD, cmd };
    i2c2_write(SSD1306_ADDR, buf, sizeof(buf));
}

/* Send two-byte command (opcode + argument) in one transaction. */
static void ssd1306_command2(uint8_t cmd, uint8_t arg)
{
    uint8_t buf[3] = { CTRL_CMD, cmd, arg };
    i2c2_write(SSD1306_ADDR, buf, sizeof(buf));
}

void ssd1306_window(uint8_t col, uint8_t page, uint8_t width, uint8_t pages)
{
    uint8_t col_end  = (uint8_t)(col + width - 1);
    uint8_t page_end = (uint8_t)(page + pages - 1);

    if (col_end >= SSD1306_WIDTH) {
        col_end = SSD1306_WIDTH - 1;
    }
    if (page_end >= SSD1306_PAGES) {
        page_end = SSD1306_PAGES - 1;
    }

    uint8_t cols[4]  = { CTRL_CMD, 0x21, col, col_end };    /* set column range */
    uint8_t pgs[4]   = { CTRL_CMD, 0x22, page, page_end };  /* set page range   */
    i2c2_write(SSD1306_ADDR, cols, sizeof(cols));
    i2c2_write(SSD1306_ADDR, pgs, sizeof(pgs));
}

void ssd1306_data(const uint8_t *buf, uint32_t len)
{
    if (i2c2_start(SSD1306_ADDR) != 0) {
        return;
    }
    i2c2_write_byte(CTRL_DATA);
    for (uint32_t i = 0; i < len; i++) {
        if (i2c2_write_byte(buf[i]) != 0) {
            break;
        }
    }
    i2c2_stop();
}

void ssd1306_init(void)
{
    i2c2_init();

    /* Datasheet power-on init for a 128x64 panel with internal charge pump. */
    ssd1306_command(0xAE);            /* display off                       */
    ssd1306_command2(0xD5, 0x80);     /* clock divide / osc freq           */
    ssd1306_command2(0xA8, 0x3F);     /* multiplex ratio = 63 (64 rows)    */
    ssd1306_command2(0xD3, 0x00);     /* display offset = 0                */
    ssd1306_command(0x40);            /* start line = 0                    */
    ssd1306_command2(0x8D, 0x14);     /* charge pump on                   */
    ssd1306_command2(0x20, 0x00);     /* memory mode = horizontal          */
    ssd1306_command(0xA1);            /* segment remap (col 127 -> SEG0)   */
    ssd1306_command(0xC8);            /* COM scan direction remapped       */
    ssd1306_command2(0xDA, 0x12);     /* COM pins config                  */
    ssd1306_command2(0x81, 0xCF);     /* contrast                         */
    ssd1306_command2(0xD9, 0xF1);     /* pre-charge period                */
    ssd1306_command2(0xDB, 0x40);     /* VCOMH deselect level             */
    ssd1306_command(0xA4);            /* resume to RAM content            */
    ssd1306_command(0xA6);            /* normal (non-inverted) display    */
    ssd1306_command(0xAF);            /* display on                       */

    ssd1306_clear();
}

void ssd1306_fill(uint8_t pattern)
{
    ssd1306_window(0, 0, SSD1306_WIDTH, SSD1306_PAGES);

    /* Stream the whole GDDRAM (128 * 8 = 1024 bytes) in one transaction. */
    if (i2c2_start(SSD1306_ADDR) != 0) {
        return;
    }
    i2c2_write_byte(CTRL_DATA);
    for (uint32_t i = 0; i < (uint32_t)SSD1306_WIDTH * SSD1306_PAGES; i++) {
        if (i2c2_write_byte(pattern) != 0) {
            break;
        }
    }
    i2c2_stop();
}

void ssd1306_clear(void)
{
    ssd1306_fill(0x00);
}

void ssd1306_text(uint8_t page, uint8_t col, const char *s)
{
    if (page >= SSD1306_PAGES) {
        return;
    }

    uint8_t x = col;
    for (; *s && x < SSD1306_WIDTH; s++) {
        unsigned char c = (unsigned char)*s;
        const uint8_t *glyph = (c >= 0x20 && c <= 0x7E)
                                   ? font5x7[c - 0x20]
                                   : font5x7[0];   /* unknown -> space */

        /* One glyph occupies 6 columns on a single page: place a tight window
         * and stream the 5 font columns plus a 1-column spacer. */
        uint8_t width = (uint8_t)(SSD1306_WIDTH - x);
        if (width > 6) {
            width = 6;
        }
        ssd1306_window(x, page, width, 1);

        uint8_t cell[6];
        cell[0] = glyph[0];
        cell[1] = glyph[1];
        cell[2] = glyph[2];
        cell[3] = glyph[3];
        cell[4] = glyph[4];
        cell[5] = 0x00;  /* inter-character spacing */
        ssd1306_data(cell, width);

        x = (uint8_t)(x + 6);
    }
}
