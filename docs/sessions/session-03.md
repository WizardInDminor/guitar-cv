# Session 03 — I2C Bring-Up (SSD1306 OLED)

**Date:** 2026-06-06  
**Status:** 🔄 Driver code written (`src/i2c.c`, `src/ssd1306.c`, `include/font5x7.h`, demo in `src/main.c`) — bench verification pending  
**Phase:** 1 — Platform Bring-Up  
**Previous session:** [Session 02 — SPI Bring-Up](session-02.md) *(SPI/DAC hardware-verified)*  
**Next session:** Session 04 — Clock/PLL + SysTick (deterministic timing)

---

## Session Goals

- Choose an I2C peripheral and pins that don't collide with onboard DISC1 hardware
- Write a minimal register-level I2C2 master driver (polling)
- Write an SSD1306 driver: init sequence, clear/fill, 5×7 text
- Render text on the OLED (Phase 1 exit criterion)
- Keep the design framebuffer-ready for the future UI

---

## Work Completed This Session

### Peripheral selection — ADR-004

I2C1's DISC1 pins (PB6/PB9) are taken by the onboard CS43L22 audio codec — the same class of
conflict that pushed SPI onto SPI2. Chose **I2C2 on PB10/PB11 (AF4)**, which keeps both
serial buses contiguous on GPIOB and on the APB1 clock domain. See
[ADR-004](../decisions/adr-004-i2c-peripheral.md).

### I2C2 driver (`src/i2c.c`)

Register-level polling master, mirroring the `spi2.c` style (inline volatile register
pointers, busy-wait on status flags). Standard mode 100 kHz at the current 16 MHz APB1
(FREQ=16, CCR=80, TRISE=17). Two layers exposed:

- `i2c2_write(addr, buf, len)` — whole transaction convenience
- `i2c2_start` / `i2c2_write_byte` / `i2c2_stop` — streaming primitives so a future
  framebuffer flush can push 1 KB with no temp buffer

Every flag wait is **timeout-bounded** (unlike the SPI driver's unbounded spins) so a
missing/miswired OLED leaves the MCU running instead of hanging on an unacknowledged
address.

### SSD1306 driver (`src/ssd1306.c`, `include/ssd1306.h`)

- Datasheet 128×64 power-on init (charge pump on, horizontal addressing)
- `ssd1306_clear()` / `ssd1306_fill()` — 1024-byte streaming blit
- `ssd1306_text(page, col, str)` — 5×7 font renderer (6 px/char, 21 chars/line, 8 rows)
- `ssd1306_window` + `ssd1306_data` low-level primitives, ready for a framebuffer layer

5×7 glyphs in `include/font5x7.h` (classic public-domain set, 0x20–0x7E).

### Demo (`src/main.c`)

Added OLED bring-up alongside the existing DAC walk: shows a "GUITAR-CV" banner and updates a
line with the current CV step (C4/C5/C6) each iteration, exercising the I2C path repeatedly.

---

## Key Decisions Made

| Decision | Choice | Reference |
|---|---|---|
| I2C peripheral | I2C2 (PB10/PB11, AF4) | [ADR-004](../decisions/adr-004-i2c-peripheral.md) |
| Bus speed | Standard mode 100 kHz | Safe bring-up; SSD1306 supports 400 kHz |
| Pull-ups | Internal + module 4.7 kΩ | Open-drain idle-high |
| Wait strategy | Timeout-bounded polling | Don't hang on a missing device |
| Display address mode | Horizontal | 1024-byte full-screen blit; framebuffer-friendly |
| Text rendering | Direct GDDRAM writes via 5×7 font | Framebuffer added later without rework |

---

## Pin / Wiring

| STM32 Pin | OLED Pin | Signal |
|---|---|---|
| PB10 | SCL | I2C2 clock (AF4, open-drain) |
| PB11 | SDA | I2C2 data (AF4, open-drain) |
| 3.3 V | VCC | Power (internal charge pump makes panel voltage) |
| GND | GND | Ground |

I2C address: 0x3C (7-bit). See [SSD1306 Wiring](../hardware/wiring-ssd1306.md).

---

## Bench Verification Plan (pending)

1. Wire the OLED on the breadboard per the table above (mind VCC/GND order on the module).
2. Build and flash.
3. Saleae I2C decode on PB10/PB11 — expect address 0x3C ACKed, command burst (control
   0x00) during init, then data bursts (control 0x40).
4. Visual: "GUITAR-CV" banner + the C4/C5/C6 line stepping with the DAC walk.
5. If I2C ACKs but the screen is dark, suspect the charge-pump command (`0x8D 0x14`) or VCC.

---

## Concepts Covered

- [I2C Peripheral Deep Dive](../concepts/i2c-peripheral.md)
- [SSD1306 OLED Driver](../firmware/ssd1306-driver.md)

---

## Session Outcome

I2C2 + SSD1306 drivers written and compiling, demo wired into `main.c`, full docs in place.
Hardware verification on the bench is the remaining step before flipping this session to ✅.

---

## Git Commit

I2C2 + SSD1306 driver + 5×7 text rendering (Session 03). Bench verification pending.
