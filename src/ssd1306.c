#include "ssd1306.h"
#include "i2c1.h"
#include "font5x7.h"

#define SSD1306_ADDR  0x3D   /* 7-bit; SA0 high. Change to 0x3C if SA0 is low */

/*
 * Send a single command byte.
 * I2C frame: [0x00 (control: command stream), cmd]
 */
static void cmd(uint8_t c)
{
    uint8_t buf[2] = { 0x00, c };
    i2c1_write(SSD1306_ADDR, buf, 2);
}

/*
 * Point the GDDRAM cursor at (page, col) in page addressing mode.  Data bytes
 * written afterwards auto-increment the column within that page.
 */
static void set_cursor(uint8_t page, uint8_t col)
{
    cmd(0xB0 | page);             /* set page address 0..7   */
    cmd(0x00 | (col & 0x0F));     /* lower column nibble     */
    cmd(0x10 | (col >> 4));       /* higher column nibble    */
}

void ssd1306_init(void)
{
    /*
     * Standard 128×64 init sequence.  Control byte 0x00 (Co=0, D/C=0)
     * tells the SSD1306 that all following bytes in this transfer are
     * commands, so we batch them in one I2C transaction.
     */
    static const uint8_t seq[] = {
        0x00,       /* control: command stream */
        0xAE,       /* display off */
        0xD5, 0x80, /* clock: fosc, divide ratio 1 */
        0xA8, 0x3F, /* mux ratio: 64 rows */
        0xD3, 0x00, /* display offset: 0 */
        0x40,       /* display start line: 0 */
        0x8D, 0x14, /* charge pump: enable */
        0x20, 0x02, /* memory addressing: page mode */
        0xA1,       /* segment remap: col 127 = SEG0 */
        0xC8,       /* COM scan direction: remapped */
        0xDA, 0x12, /* COM pins: alternative config, no remap */
        0x81, 0xCF, /* contrast */
        0xD9, 0xF1, /* pre-charge period */
        0xDB, 0x40, /* VCOMH deselect level */
        0xA4,       /* display follows RAM content */
        0xA6,       /* normal (not inverted) */
        0xAF,       /* display on */
    };
    i2c1_write(SSD1306_ADDR, seq, sizeof(seq));
}

void ssd1306_fill(uint8_t pattern)
{
    /*
     * In page addressing mode we set the cursor (page, column 0) then write
     * 128 data bytes for that row.  Repeat for all 8 pages.
     *
     * Data frame: [0x40 (control: data stream), 128× pattern]
     */
    static uint8_t row[129];  /* 1 control byte + 128 pixel bytes */
    row[0] = 0x40;
    for (int i = 1; i <= 128; i++) {
        row[i] = pattern;
    }

    for (uint8_t page = 0; page < 8; page++) {
        set_cursor(page, 0);
        i2c1_write(SSD1306_ADDR, row, sizeof(row));
    }
}

void ssd1306_text(uint8_t page, uint8_t col, const char *s)
{
    if (page >= 8) {
        return;
    }

    /*
     * Each glyph is 5 font columns plus 1 blank column of spacing, and sits
     * entirely within one page — so a glyph is just a cursor move followed by
     * a 6-byte data write.  No frame buffer needed.
     */
    uint8_t x = col;
    for (; *s != '\0' && x < 128; s++) {
        unsigned char c = (unsigned char)*s;
        if (c < 0x20 || c > 0x7E) {
            c = 0x20;                       /* unprintable renders as space */
        }
        const uint8_t *glyph = font5x7[c - 0x20];

        uint8_t cell[7];                    /* control byte + 5 font + 1 gap */
        cell[0] = 0x40;                     /* control: data stream          */
        for (uint8_t i = 0; i < 5; i++) {
            cell[1 + i] = glyph[i];
        }
        cell[6] = 0x00;                     /* inter-character spacing       */

        /*
         * Clip at the right edge: send only the columns that still fit rather
         * than letting the cursor wrap onto the start of the same page.
         */
        uint8_t cols = (uint8_t)(128u - x);
        if (cols > 6) {
            cols = 6;
        }

        set_cursor(page, x);
        i2c1_write(SSD1306_ADDR, cell, (uint16_t)(1 + cols));

        x = (uint8_t)(x + 6);
    }
}
