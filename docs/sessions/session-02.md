# Session 02 — SPI Bring-Up (MCP4922 DAC)

**Date:** 2026-04-10 (bench verification 2026-06-05)  
**Status:** ✅ Complete — driver code written (`src/spi2.c`, `src/dac.c`, bench demo in `src/main.c`) and **hardware-verified on 2026-06-05** via Saleae Logic 2 MSO (SPI decode + analog scope)  
**Phase:** 1 — Platform Bring-Up  
**Previous session:** Session 01 — Bare-metal blink, toolchain, GDB *(notes not yet written)*  
**Next session:** Session 03 — I2C Bring-Up (SSD1306 OLED)

---

## Session Goals

- Understand STM32 SPI peripheral registers
- Understand the STM32 alternate function system
- Select and justify SPI peripheral and pin assignment
- Write minimal SPI transmit function
- Send a known value to the MCP4922
- Verify output voltage on oscilloscope
- Understand 1V/oct CV mapping math

---

## Work Completed This Session

### Developer Workflow Setup

Before beginning SPI work, the development workspace was formalized:

- **Tmuxinator project configured** at `~/dev/dotfiles/config/tmuxinator/guitar-cv.yml`
- Three-window layout: `code` (Neovim + build shell), `debug` (OpenOCD auto-start + GDB auto-connect), `docs` (MkDocs serve + writing shell)
- GDB pane uses `nc` polling loop to wait for OpenOCD before connecting — eliminates race condition on session start
- `tio` pane monitoring `/dev/ttyACM0` confirmed connected (ST-LINK virtual COM port)
- **MkDocs with Material theme** initialized and serving at `http://127.0.0.1:8000`
- Full documentation structure established for the project

### Conceptual Work — SPI and MCP4922

The full conceptual foundation for Session 02 bench work was completed:

1. **MCP4922 datasheet review** — timing diagram (Figure 5.1) analyzed directly
2. **SPI peripheral selection** — schematic traced to confirm SPI2 on PB12–PB15 is free
3. **STM32 SPI register map** — CR1, SR, DR understood at the bit level
4. **Alternate function system** — four-layer configuration model understood
5. **Complete bring-up sequence** — all register writes documented with reasoning
6. **1V/oct math** — formula derived, reference table generated

---

## Key Decisions Made

| Decision | Choice | ADR |
|---|---|---|
| SPI peripheral | SPI2 (PB13/PB15) | [ADR-001](../decisions/adr-001-spi-peripheral.md) |
| CS management | Software GPIO (PB12) | [ADR-002](../decisions/adr-002-cs-management.md) |
| SPI clock rate | ÷8 = 2MHz (bring-up) | [ADR-003](../decisions/adr-003-spi-clock-rate.md) |
| Data frame | 16-bit (DFF=1) | Matches MCP4922 command word size |
| LDAC | Tied low permanently | Immediate output update on CS rising edge |

---

## Schematic Investigation Notes

Confirming pin availability required tracing the DISC1 MB997E schematic:

- **U2 (STM32F103CBT6)** — the ST-LINK controller, not our target. Its PB13/PB14 connect to JTAG signals going *to* the F407. This is unrelated to our SPI2.
- **U4A (STM32F407VGT6)** — our target. PB12–PB15 confirmed routed to the P1 expansion header with no connections to onboard peripherals.
- **LIS3DSH accelerometer** — consumes PA5 (SCL), PA6 (SEL), PA7 (SDA) — eliminates SPI1 in its default pin mapping.
- **CS43L22 audio codec** — control interface is I2C on PB6/PB9, not relevant to SPI selection.

!!! warning "Common Trap"
    The STM32F103 ST-LINK controller (U2) appears in the schematic with PB13/PB14 connected to JTAG signals. These are the F103's pins, not the F407's. Always confirm which chip's pins you are tracing.

---

## Bench Work Plan (Tomorrow)

### Wiring

| STM32 Pin | MCP4922 Pin | Signal |
|---|---|---|
| PB13 | 3 — SCK | SPI clock |
| PB15 | 4 — SDI | SPI data |
| PB12 | 2 — CS | Chip select |
| GND | 5 — LDAC | Tied low |
| 3.3V | 1 — VDD | Power |
| GND | 9 — VSS | Ground |
| 3.3V | 13 — VREFA | Reference voltage |

### Test Sequence

1. Wire MCP4922 on breadboard per table above
2. Build and flash SPI init + known DAC value
3. Probe CS, SCK, MOSI on oscilloscope — verify waveform before measuring output
4. Confirm 16 clock pulses per transaction
5. Confirm CS behavior (low before SCK, high after 16th pulse)
6. Probe DAC output (VOUTA, pin 8)
7. Send count = 0 → expect ~0V
8. Send count = 1241 → expect ~1.000V (C5, one octave above reference)
9. Send count = 2483 → expect ~2.000V (C6, two octaves above reference)
10. Verify voltage steps in correct direction and approximate magnitude

### First Scope Measurements

| DAC Count | Expected Voltage | Notes |
|---|---|---|
| 0 | 0.000V | Reference level |
| 1241 | 1.000V | C5 — cleanest test point |
| 2483 | 2.000V | C6 |

!!! note "Calibration"
    Exact voltages will deviate slightly from 3.3V reference assumptions. This is expected — calibration is a separate session. Tonight's goal is correct direction and approximate magnitude.

---

## Bench Verification Results (2026-06-05)

Initial SPI bring-up and MCP4922 DAC conversion **verified in hardware** using a Saleae
Logic 2 (MSO — mixed-signal: SPI protocol decode on the digital channels plus the analog
scope on the DAC output). Analyzer configured per
[Saleae Logic 2 — SPI Decode Setup](../hardware/saleae-logic2-spi-setup.md)
(Mode 0, MSB-first, 16-bit, active-low CS).

The captured SPI words decoded exactly as designed, and the measured analog output on VOUTA
matched the expected 1V/oct levels:

| Sent | Decoded word | DAC count | Channel | Measured VOUTA | Expected |
|---|---|---|---|---|---|
| C4 / zero | `0x3000` | 0 | A | **0 V** | 0.000 V |
| C5 | `0x34D9` | 1241 | A | **1 V** | 1.000 V (one octave up) |
| C6 | `0x39B2` | 2482 | A | **2 V** | 2.000 V (two octaves up) |

Each word carries the expected control nibble `0b0011` (Channel A, unbuffered, 1× gain,
active). Digital checks from the capture all passed: SCK idles low (CPOL=0), 16 clock
pulses per frame, CS asserts low before SCK and returns high after the 16th pulse, and the
analog output latches to the new level on the CS rising edge (LDAC tied to GND).

!!! success "SPI → MCP4922 chain validated"
    This closes out the highest-value Phase 1 milestone — the documented SPI/DAC design is
    now proven working firmware on real hardware. Exact voltages are clean enough that
    formal calibration can be deferred to a later session as planned.

---

## Concepts Covered

- [STM32 Alternate Function System](../concepts/stm32-alternate-functions.md)
- [SPI Peripheral Deep Dive](../concepts/spi-peripheral.md)
- [CV Math — 1V/oct](../concepts/cv-math.md)

---

## Session Outcome

Conceptual phase complete, all firmware decisions made and documented, and the SPI →
MCP4922 chain **verified in hardware** on 2026-06-05 (see Bench Verification Results above).
DAC output measured at 0 V / 1 V / 2 V for counts 0 / 1241 / 2482. Phase 1 SPI/DAC
milestone done.

---

## Git Commit

SPI bring-up + DAC conversion hardware-verified via Saleae Logic 2 MSO (2026-06-05).
