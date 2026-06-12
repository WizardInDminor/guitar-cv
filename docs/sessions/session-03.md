# Session 03 — I2C Bring-Up (SSD1306 OLED)

**Date:** 2026-06-11  
**Status:** Driver code written (`src/i2c1.c`, `src/ssd1306.c`) — bench verification pending  
**Phase:** 1 — Platform Bring-Up  
**Previous session:** [Session 02 — SPI Bring-Up (MCP4922 DAC)](session-02.md)  
**Next session:** Session 04 — SysTick / Timer

---

## Session Goals

- Understand STM32 I2C peripheral registers and the event-driven state machine
- Understand I2C bus electrical requirements (open-drain, pull-ups)
- Write minimal polling I2C1 master driver (`i2c1_write`)
- Write SSD1306 OLED init and fill functions
- Verify solid white screen on the OLED at power-up

---

## Work Completed This Session

### Drivers Written

**`src/i2c1.c` / `include/i2c1.h`** — peripheral driver:

- `i2c1_init()` — configures PB6 (SCL) and PB7 (SDA) as AF4 open-drain with internal
  pull-ups; sets standard-mode timing (CCR=80, TRISE=17) for 100 kHz at 16 MHz APB1;
  software-resets the peripheral before configuring (CCR/TRISE require PE=0)
- `i2c1_write(addr7, buf, len)` — polling state machine: wait BUSY → START → wait SB →
  send address → wait ADDR (cleared by SR1+SR2 read) → send bytes waiting TxE → wait
  BTF → STOP

**`src/ssd1306.c` / `include/ssd1306.h`** — device driver:

- `ssd1306_init()` — sends the full 128×64 init sequence in one I2C transaction using
  control byte `0x00` (command stream); configures page addressing mode
- `ssd1306_fill(pattern)` — for each of 8 pages: sets page/column cursor, sends 128 data
  bytes; `0xFF` = white, `0x00` = black

**`src/main.c`** — `i2c1_init()` + `ssd1306_init()` + `ssd1306_fill(0xFF)` added before
the DAC demo loop. Power-up: solid white display, then the DAC CV-walk resumes.

### Docs Written

- `docs/firmware/i2c-driver.md` — I2C peripheral configuration, state machine, SSD1306
  framing, verification checklist
- `docs/hardware/wiring-ssd1306.md` — pin assignment, connection table, mermaid wiring diagram
- `docs/reference/register-map.md` — I2C1 register table added

---

## Key Decisions Made

| Decision | Choice | Reasoning |
|---|---|---|
| I2C peripheral | I2C1 | PB6/PB7 available on expansion header; I2C2 (PB10/11) reserved for future use |
| Pins | PB6 (SCL), PB7 (SDA) | Standard I2C1 mapping (AF4); both on P1 header |
| Speed | 100 kHz standard mode | Conservative for breadboard bring-up wires |
| Pull-ups | Internal (bring-up) | No external resistors on breadboard yet; replace with 4.7 kΩ on final board |
| SSD1306 addressing | Page mode (0x20, 0x02) | Simpler page-by-page writes; 128-byte row buffer fits on stack |
| I2C address | 0x3C | SA0 low (default on most modules); change to 0x3D in `ssd1306.c` if needed |

---

## Schematic Notes

- **PB6** is also used by the onboard **CS43L22 audio codec** as I2C1_SCL. No conflict: the
  codec's SDA is on PB9 (not PB7), and we never initialize the codec in our firmware.
- **PB7** is free — not connected to any onboard DISC1 peripheral.
- The I2C1 lines go to the P1 expansion header; see UM1472 Table 11 for exact pin rows.

---

## Bench Work Plan

### Wiring

| SSD1306 pin | STM32 |
|---|---|
| GND | GND (P1-49/50) |
| VCC | 3.3 V (P2-5/6) |
| SCL | PB6 (check UM1472 Table 11 for header row) |
| SDA | PB7 (check UM1472 Table 11 for header row) |

### Test Sequence

1. Wire SSD1306 module per table above
2. Build and flash: `make && make flash`
3. On power-up, display should show **solid white** within ~100 ms
4. If no display: probe SCL/SDA with logic analyzer
   - Confirm lines idle high (pull-ups present)
   - Confirm address byte `0x78` is ACK'd by module
5. Send `ssd1306_fill(0x00)` → confirm black screen (no dead pixels)
6. Send `ssd1306_fill(0xAA)` → confirm checkerboard (pixel-level test)

---

## I2C State Machine — Quick Reference

The STM32F407 I2C generates events that must be polled in order. Missing a step or reading
the wrong register will stall the bus.

| Step | Action | Flag to poll | Clear by |
|---|---|---|---|
| 1 | Wait bus free | SR2.BUSY = 0 | (automatic after STOP) |
| 2 | Set CR1.START | SR1.SB = 1 | Reading SR1, then writing DR |
| 3 | Write address to DR | SR1.ADDR = 1 | Reading SR1, then reading SR2 |
| 4 | Write data byte | SR1.TxE = 1 | Writing next byte to DR |
| 5 | (repeat step 4) | — | — |
| 6 | Wait last byte clocked out | SR1.BTF = 1 | Setting CR1.STOP |

---

## Concepts Covered

- [I2C Driver](../firmware/i2c-driver.md)
- [SSD1306 Wiring](../hardware/wiring-ssd1306.md)

---

## Session Outcome

Code written and firmware builds cleanly. Bench verification pending — wire up the OLED
module and confirm solid white screen on the next bench session.
