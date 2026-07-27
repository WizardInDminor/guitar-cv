# System Requirements

## MVP Requirements

These are the minimum requirements for capstone success.

### Functional Requirements

| ID | Requirement | Status |
|---|---|---|
| FR-01 | Accept instrument/line-level guitar input | Not started |
| FR-02 | Detect monophonic pitch over full guitar range | Not started |
| FR-03 | Detect note onset/envelope for gate generation | Not started |
| FR-04 | Output 1V/oct CV in real time | In progress — DAC output path implemented and bench-verified (2026-06-05); awaits live pitch source |
| FR-05 | Output gate signal for note activity | Not started |
| FR-06 | Record and store pitch/gate sequences | Not started |
| FR-07 | Support quantized sequence storage | Not started |
| FR-08 | Support unquantized sequence storage | Not started |
| FR-09 | Tap tempo or definable tempo control | Not started |
| FR-10 | Basic UI for mode selection and parameter interaction | Not started |
| FR-11 | Display feedback for settings, state, sequence info | Not started |

### Hardware Requirements

| ID | Requirement | Status |
|---|---|---|
| HR-01 | MCU: STM32F405RG (final target) | Not yet sourced (developing on STM32F407G-DISC1) |
| HR-02 | External DAC: MCP4922 via SPI | ✅ Driver written + hardware-verified 2026-06-05 |
| HR-03 | Display: SSD1306 OLED via I2C | ✅ Driver written + hardware-verified 2026-06-12 (fill only; text rendering pending) |
| HR-04 | Physical controls: encoder + buttons | Not started |
| HR-05 | Eurorack-conscious power design | Not started (AFE constraint fixed: 3.3 V single-supply analog, ADR-005) |
| HR-06 | Analog front end: Rev A.1 — 3.3 V single-supply TLV9062 ([ADR-005](../decisions/adr-005-analog-front-end.md)) | Design approved; build not started |

---

## Performance Targets

These are engineering targets, not guaranteed specifications.

| Parameter | Target |
|---|---|
| Pitch detection latency | Low enough to feel like an instrument |
| CV output stability | Stable enough to drive external oscillators predictably |
| Note tracking | Stable monophonic tracking across guitar range |
| Low note support — **core** | Standard guitar low E2 ≈ 82.41 Hz (period ≈ 12.1 ms) |
| Low note support — **stretch** | Five-string-bass low B0 ≈ 30.87 Hz (period ≈ 32.4 ms) |
| ADC sample rate | 24 kHz target (Rev A.1 AFE, ADR-005) |
| Gate feel | Musically coherent onset/offset behavior |

### Supported Input Range

The **core requirement is standard six-string guitar**: lowest fundamental is low E2 at
≈ 82.41 Hz. Extending down to five-string-bass low B0 (≈ 30.87 Hz) is a **stretch
goal**, not the MVP minimum — a detector observing ~3 periods needs roughly **36 ms**
of signal at low E2 but roughly **97 ms** at low B0, a product-level latency
difference. Earlier documents framed low B as the aspirational low end; this table is
the normative statement.

---

## Success Criteria

### Minimum Success
- Guitar input received and processed by the system
- Monophonic pitch detected in real time
- CV and gate generated from detected pitch
- Quantized and unquantized sequence handling demonstrated
- Basic UI interaction working
- Live demonstration with guitar and modular gear

### Strong Success
- Tracking stable enough for convincing demonstration
- Clean, documented firmware architecture
- Hardware and firmware decisions well justified
- Final report explains tradeoffs, implementation, limitations, and future work

---

## Out of Scope (MVP)

- Polyphonic pitch detection
- Full DAW-style sequencing
- Complex graphical UI
- Large preset management
- Commercial enclosure or industrial design polish
- Full manufacturing readiness
