#include "ssd1306.h"
#include "i2c1.h"

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
        cmd(0xB0 | page);   /* set page address 0..7 */
        cmd(0x00);          /* lower column nibble = 0 */
        cmd(0x10);          /* higher column nibble = 0 */
        i2c1_write(SSD1306_ADDR, row, sizeof(row));
    }
}
