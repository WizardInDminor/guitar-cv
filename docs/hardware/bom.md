# Bill of Materials

## Development Platform

| Component | Quantity | Status | Notes |
|---|---|---|---|
| STM32F407G-DISC1 | 1 | ✅ Verified | Development board |
| ST-LINK/V2.1 | 1 | ✅ Verified | Built into DISC1 |

## Core Components

| Component | Quantity | Status | Notes |
|---|---|---|---|
| STM32F405RG | 1 | Not yet sourced | Final target MCU |
| MCP4922 | 2 | On hand | 12-bit dual SPI DAC |
| SSD1306 OLED | 1 | On hand | 128×64 I2C display |

## Analog Components — Rev A.1 AFE

Current front-end design per [ADR-005](../decisions/adr-005-analog-front-end.md):
3.3 V single-supply, TLV9062-based.

| Component | Quantity | Status | Notes |
|---|---|---|---|
| TLV9062 Dual Op-Amp | 1 | To source | VBIAS buffer + ≈4.9× gain stage (rail-to-rail, 3.3 V) |
| 10 kΩ resistor | 3 | On hand | VBIAS divider (×2), gain Rg |
| 39 kΩ resistor | 1 | On hand | Gain Rf |
| 1 MΩ resistor | 1 | On hand | Post-coupling bias / input impedance |
| 2.2 kΩ resistor | 1 | On hand | Input series protection |
| 3.3 kΩ resistor | 1 | On hand | ADC filter |
| 100 nF capacitor | 2 | On hand | AC coupling; VBIAS filter |
| 10 µF capacitor | 1 | On hand | VBIAS filter |
| 220 pF capacitor | 1 | On hand | RF shunt |
| 10 nF capacitor | 1 | On hand | ADC filter |
| SN74HC14 Schmitt Trigger | 3 | On hand | Signal conditioning (gate/clock I/O) |
| 2N3904 NPN Transistor | 2 | On hand | Gate output drive |
| 1N4148 Diode | 10 | On hand | Protection |

### Superseded (not used in Rev A.1)

Retained as bench stock only — superseded by ADR-005:

| Component | Quantity | Status | Notes |
|---|---|---|---|
| TL074 Quad Op-Amp | 5 | On hand | ~~Analog front end~~ superseded |
| TL072 Dual Op-Amp | 2 | On hand | ~~Analog front end~~ superseded |
| TLE2426 Virtual GND | 2 | On hand | ~~Rail splitter / virtual ground~~ superseded |

## Connectors and Passives

| Component | Quantity | Status | Notes |
|---|---|---|---|
| 3.5mm Mono Jack | 5 | On hand | Eurorack I/O |
