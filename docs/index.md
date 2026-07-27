# Guitar-to-CV Sequencer

A bare-metal STM32 capstone: real-time **monophonic guitar pitch → Eurorack CV/gate**,
with an onboard step sequencer and a small OLED + encoder/button UI. Built register-level
in C, no HAL — every peripheral configured and reasoned about explicitly.

## Start here

- **[Project Roadmap & Onboarding](roadmap.md)** — what the project is, where it stands, and the phased build plan.
- **[Charter](system/charter.md)** · **[Requirements](system/requirements.md)** · **[System Overview](system/overview.md)**

## Hardware

- **[MCP4922 Wiring (datasheet-verified)](hardware/wiring-mcp4922.md)** — DAC ↔ STM32 connections.
- **[SSD1306 Wiring](hardware/wiring-ssd1306.md)** — OLED ↔ STM32 I2C connections.
- **[Analog Front End — Rev A.1](hardware/analog-front-end.md)** — 3.3 V TLV9062 design ([ADR-005](decisions/adr-005-analog-front-end.md)).
- [Power Design](hardware/power.md) · [Bill of Materials](hardware/bom.md)

## Firmware

- [Architecture](firmware/architecture.md) · [Clock (PLL 168 MHz)](firmware/clock.md) · [SPI Driver](firmware/spi-driver.md) · [DAC Output](firmware/dac-output.md) · [I2C Driver](firmware/i2c-driver.md) · [Timing (SysTick)](firmware/timing.md)
- Concepts: [STM32 Alternate Functions](concepts/stm32-alternate-functions.md) · [SPI Peripheral Deep Dive](concepts/spi-peripheral.md) · [CV Math — 1V/oct](concepts/cv-math.md)

## Reference & decisions

- [Register Map](reference/register-map.md) · [MCP4922 Command Word](reference/mcp4922-command-word.md)
- ADRs: [001 SPI Peripheral](decisions/adr-001-spi-peripheral.md) · [002 CS Management](decisions/adr-002-cs-management.md) · [003 SPI Clock Rate](decisions/adr-003-spi-clock-rate.md) · [005 Analog Front End](decisions/adr-005-analog-front-end.md)

## Build the docs

```sh
mkdocs serve   # live preview at http://127.0.0.1:8000
mkdocs build   # static site into ./site
```
