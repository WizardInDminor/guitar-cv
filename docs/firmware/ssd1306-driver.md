# SSD1306 OLED Driver

Driver for the 128×64 SSD1306 OLED over I2C2. Provides display init, screen fill/clear, and
a 5×7 text renderer. Built on the [I2C driver](../concepts/i2c-peripheral.md); see
[ADR-004](../decisions/adr-004-i2c-peripheral.md) for the peripheral choice and
[SSD1306 Wiring](../hardware/wiring-ssd1306.md) for the connections.

Source: `src/i2c.c`, `src/ssd1306.c`, `include/font5x7.h`.

---

## Layering

```
main.c
  └─ ssd1306_text / ssd1306_clear / ssd1306_fill   (src/ssd1306.c)
       └─ ssd1306_command / ssd1306_window / ssd1306_data
            └─ i2c2_write / i2c2_start+write_byte+stop   (src/i2c.c)
```

The SSD1306 layer never touches registers directly — it only frames bytes and hands them to
the I2C driver, exactly as the DAC layer sits on top of SPI.

---

## Pin assignment

| Signal | STM32 pin | AF | OLED pin |
|---|---|---|---|
| SCL | PB10 | AF4 | SCL/D0 |
| SDA | PB11 | AF4 | SDA/D1 |
| 3.3 V | — | — | VCC |
| GND | — | — | GND |

I2C address: **0x3C** (7-bit). Some modules strap to 0x3D — change `SSD1306_ADDR` if so.

---

## Display geometry

The panel is 128 columns × 64 rows, organised as **8 pages** of 8 vertical pixels:

```
        col 0 ............................. col 127
page 0  ████ each byte = 8 vertical pixels, LSB on top
page 1
 ...
page 7
```

A byte written to GDDRAM paints one 8-pixel-tall column slice. In **horizontal addressing
mode** (used here), the column auto-increments after each byte and wraps to the next page,
so a full-screen blit is just 1024 sequential data bytes.

---

## Command vs data framing

After the I2C address, the first byte is an SSD1306 **control byte**:

| Control byte | Meaning |
|---|---|
| 0x00 | Command byte(s) follow |
| 0x40 | Data (GDDRAM pixel) bytes follow |

`ssd1306_command()` sends `[0x00, cmd]`; `ssd1306_data()` streams `[0x40, b0, b1, …]`.

---

## Initialization sequence

`ssd1306_init()` brings up I2C2, then issues the datasheet power-on sequence for a 128×64
panel with the internal charge pump:

```c
ssd1306_command(0xAE);          // display off
ssd1306_command2(0xD5, 0x80);   // clock divide / osc freq
ssd1306_command2(0xA8, 0x3F);   // multiplex ratio = 63 (64 rows)
ssd1306_command2(0xD3, 0x00);   // display offset = 0
ssd1306_command(0x40);          // start line = 0
ssd1306_command2(0x8D, 0x14);   // charge pump on
ssd1306_command2(0x20, 0x00);   // memory mode = horizontal
ssd1306_command(0xA1);          // segment remap (col 127 -> SEG0)
ssd1306_command(0xC8);          // COM scan direction remapped
ssd1306_command2(0xDA, 0x12);   // COM pins config
ssd1306_command2(0x81, 0xCF);   // contrast
ssd1306_command2(0xD9, 0xF1);   // pre-charge
ssd1306_command2(0xDB, 0x40);   // VCOMH deselect
ssd1306_command(0xA4);          // resume to RAM content
ssd1306_command(0xA6);          // normal display
ssd1306_command(0xAF);          // display on
ssd1306_clear();
```

!!! warning "Charge pump"
    The DISC1 only supplies 3.3 V; the panel's ~7 V drive comes from the SSD1306's internal
    charge pump. Command `0x8D 0x14` **must** enable it or the screen stays dark even though
    I2C ACKs correctly.

---

## Text rendering

`ssd1306_text(page, col, "string")` draws each character from the 5×7 font in
`include/font5x7.h`. Each glyph occupies **6 columns** (5 font + 1 spacing) on a single
page, so a line holds 21 characters and there are 8 text rows (pages 0–7).

```c
ssd1306_text(0, 0, "GUITAR-CV");
ssd1306_text(2, 0, "C5  1.000V");
```

Characters outside 0x20–0x7E render as blanks; text is clipped at the right edge.

---

## Framebuffer-ready by design

The user-facing UI will eventually want pixel/line drawing. This driver is structured so
that a RAM framebuffer drops in **without reworking this layer**:

- `ssd1306_data()` streams an arbitrary-length byte buffer straight to GDDRAM via the I2C
  streaming primitives — no temporary buffer, so a 1024-byte flush is one call.
- A future `gfx` layer keeps `uint8_t fb[1024]`, draws into it, then flushes with:

```c
ssd1306_window(0, 0, 128, 8);   // whole screen
ssd1306_data(fb, 1024);         // one transaction
```

`ssd1306_fill()` already exercises exactly this 1024-byte streaming path.

---

## Verification

1. Wire the OLED per [SSD1306 Wiring](../hardware/wiring-ssd1306.md).
2. Build + flash; the bench demo in `src/main.c` shows a banner and the current CV step.
3. On the Saleae, decode I2C on PB10/PB11 — expect address **0x3C** ACKed, a burst of
   command bytes (control 0x00), then data bursts (control 0x40).
4. Visually: "GUITAR-CV" banner with the C4/C5/C6 line updating in step with the DAC walk.
