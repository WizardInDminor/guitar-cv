# SSD1306 OLED Driver

## Overview

Minimal write-only driver for the 128×64 SSD1306 OLED. Provides display init, a full-screen
fill, and a 5×7 text renderer. It sits entirely on top of the [I2C Driver](i2c-driver.md) and
never touches I2C registers itself — the same layering the DAC uses over SPI.

See also: [SSD1306 Wiring](../hardware/wiring-ssd1306.md), [Register Map](../reference/register-map.md)

Source: `src/ssd1306.c`, `include/ssd1306.h`, `include/font5x7.h`.

---

## Layering

```
main.c
  └─ ssd1306_init / ssd1306_fill / ssd1306_text   (src/ssd1306.c)
       └─ cmd() / set_cursor()
            └─ i2c1_write()                        (src/i2c1.c)
```

---

## Pin Assignment

Inherited from the I2C driver — see [I2C Driver](i2c-driver.md) for the full configuration
and the note on the shared PB6 codec pin.

| Signal | Pin | AF | OLED pin |
|---|---|---|---|
| I2C1_SCL | PB6 | AF4 | SCL / D0 |
| I2C1_SDA | PB7 | AF4 | SDA / D1 |
| 3.3 V | — | — | VCC |
| GND | — | — | GND |

I2C address: **0x3D** (7-bit, SA0 strapped high on the bench module). Modules that tie SA0
low answer at 0x3C — change `SSD1306_ADDR` in `src/ssd1306.c` if so.

---

## Display Geometry

The panel is 128 columns × 64 rows, organised as **8 pages** of 8 vertical pixels:

```
        col 0 ............................. col 127
page 0  ████ each byte = 8 vertical pixels, LSB on top
page 1
 ...
page 7
```

One byte written to GDDRAM paints an 8-pixel-tall column slice.

!!! note "Page addressing mode"
    This driver uses **page addressing mode** (`0x20 0x02`), not horizontal mode. The column
    cursor auto-increments after each data byte but **wraps within the current page** instead
    of spilling into the next one. Every write therefore begins with an explicit
    `set_cursor(page, col)`, and nothing can accidentally bleed across page boundaries.

---

## Command vs Data Framing

After the I2C address, the first byte is an SSD1306 **control byte**:

| Control byte | Meaning |
|---|---|
| 0x00 | Command byte(s) follow |
| 0x40 | Data (GDDRAM pixel) bytes follow |

`cmd()` sends `[0x00, c]`. `set_cursor()` is three commands — `0xB0|page`, `0x00|(col & 0x0F)`
(low nibble), `0x10|(col >> 4)` (high nibble).

---

## Initialization Sequence

`ssd1306_init()` issues the datasheet power-on sequence for a 128×64 panel as a single
batched transaction (one control byte, then every command). `i2c1_init()` must be called
first — the driver does not bring up the bus itself.

!!! warning "Charge pump"
    The DISC1 only supplies 3.3 V; the panel's ~7 V drive comes from the SSD1306's internal
    charge pump. Command `0x8D 0x14` **must** enable it, or the screen stays dark even though
    I2C ACKs correctly.

GDDRAM contents are undefined at power-on, so call `ssd1306_fill(0x00)` before drawing text.

---

## Text Rendering

`ssd1306_text(page, col, "string")` draws each character from the 5×7 font in
`include/font5x7.h` (95 glyphs, ASCII 0x20–0x7E, column-major, LSB = top pixel).

Each glyph occupies **6 columns** — 5 font columns plus 1 blank spacer — and sits entirely
within one page. A glyph is therefore just a cursor move followed by a 6-byte data write, so
no frame buffer is needed. A line holds 21 characters; there are 8 text rows (pages 0–7).

```c
ssd1306_fill(0x00);
ssd1306_text(0, 0, "GUITAR-CV");
ssd1306_text(2, 0, "C5  1.000V");
```

| Behaviour | Detail |
|---|---|
| `page >= 8` | Call is a no-op |
| Characters outside 0x20–0x7E | Render as blanks |
| Right edge | Clipped — the partial glyph is truncated, it does not wrap |
| Background | **Not** erased — overwrite with equal-width text, or `ssd1306_fill(0x00)` first |

!!! note "Inherited blocking behaviour"
    `i2c1_write()` currently polls with unbounded `while` loops, so a missing or unresponsive
    panel will hang the MCU rather than time out. Text rendering inherits this. Adding
    bounded waits and NACK detection to the I2C layer is tracked as future work.

---

## Verification

Host-side, with no hardware attached (the ARM toolchain and panel are not needed):

- `cc -fsyntax-only -Wall -Wextra -Iinclude src/ssd1306.c` — clean under `-Wextra`.
- A throwaway harness under `build/` stubs `i2c1_write()`, emulates page-mode GDDRAM, and
  dumps the result as ASCII art. This confirms glyph legibility and orientation, the
  1-column spacer, full 1024-byte `fill()` coverage, right-edge clipping without wrap, and
  the `page >= 8` / unprintable-character guards.

On hardware:

1. Wire the OLED per [SSD1306 Wiring](../hardware/wiring-ssd1306.md).
2. Build and flash; the bench demo in `src/main.c` shows a banner plus the current CV step.
3. On the Saleae, decode I2C on PB6/PB7 — expect address **0x3D** ACKed, a burst of command
   bytes (control 0x00), then data bursts (control 0x40).
4. Visually: a "GUITAR-CV" banner with the C4/C5/C6 line updating in step with the DAC walk.
