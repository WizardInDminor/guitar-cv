# System Overview

## Block Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        Guitar-to-CV Sequencer                   │
│                                                                 │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐  │
│  │  Guitar  │───▶│ Analog   │───▶│  STM32   │───▶│ MCP4922  │──┼──▶ CV Out
│  │  Input   │    │ Front    │    │  F407    │    │   DAC    │  │
│  └──────────┘    │  End     │    │          │    └──────────┘  │
│                  └──────────┘    │          │                  │
│                                  │          │───▶ Gate Out     │
│  ┌──────────┐                    │          │                  │
│  │ Encoder  │───▶────────────────│          │───▶ SSD1306      │
│  │ Buttons  │                    │          │     OLED         │
│  └──────────┘                    └──────────┘                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Subsystem Descriptions

### Analog Front End
Conditions the instrument-level guitar signal for ADC input. Handles gain staging, DC biasing, and signal protection. Designed around TL072/TL074 op-amps with TLE2426 virtual ground reference.

See: [Analog Front End](../hardware/analog-front-end.md)

### STM32F407 Control Core
The central processing element. Runs bare-metal C firmware responsible for:

- ADC sample capture
- Pitch detection (YIN algorithm)
- Envelope and onset detection
- Sequence engine
- CV/gate output control
- UI and display management

See: [Firmware Architecture](../firmware/architecture.md)

### MCP4922 DAC
Dual-channel 12-bit SPI DAC. Converts digital pitch values from the MCU into analog control voltages following the 1V/oct convention.

See: [DAC Output](../firmware/dac-output.md), [MCP4922 Command Word](../reference/mcp4922-command-word.md)

### Gate Output
Digital output signal indicating note-on/note-off state. Driven by envelope detection in firmware. Compatible with Eurorack gate/trigger inputs.

### SSD1306 OLED Display
128×64 pixel I2C display. Provides mode, state, and sequence feedback to the user.

### User Controls
Physical encoder and buttons for mode selection, parameter adjustment, and performance interaction.

---

## Signal Flow

```
Guitar signal
  → Analog front end (gain, bias, protection)
  → STM32 ADC (continuous sampling)
  → Pitch detection (YIN algorithm)
  → Note quantization (if enabled)
  → CV mapping (note → DAC count via 1V/oct math)
  → MCP4922 SPI write
  → Analog CV output → Eurorack oscillator

Guitar signal
  → Analog front end
  → Envelope detection
  → Gate logic (onset/offset detection)
  → Gate output → Eurorack envelope/trigger
```

---

## Design Philosophy

This project prioritizes understanding over speed. Every peripheral is configured at the register level with explicit reasoning for each decision. The firmware architecture is designed to remain maintainable as features grow, with clean separation between hardware drivers, signal processing, control logic, and UI.
