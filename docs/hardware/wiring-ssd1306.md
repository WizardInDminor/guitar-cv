# SSD1306 OLED Wiring

Wiring for the 128×64 SSD1306 OLED (I2C variant) to the STM32F407G-DISC1. The display gives
UI feedback (mode/state/sequence). Peripheral and pin choice are justified in
[ADR-004](../decisions/adr-004-i2c-peripheral.md); the driver is documented in
[SSD1306 OLED Driver](../firmware/ssd1306-driver.md).

See also: [MCP4922 Wiring](wiring-mcp4922.md), [I2C Peripheral Deep Dive](../concepts/i2c-peripheral.md).

---

## Module pinout (4-pin I2C OLED)

Most 128×64 I2C SSD1306 breakout boards expose four pins:

```
┌─────────────────────┐
│   SSD1306 128x64     │
│   GND  VCC  SCL  SDA │
└────┬────┬────┬────┬──┘
     │    │    │    │
    GND  3V3  PB10 PB11
```

!!! note "Pin order varies"
    Some modules order the header `VCC GND SCL SDA` instead of `GND VCC SCL SDA`. Check the
    silkscreen — swapping VCC/GND can damage the module.

---

## Connection table

| OLED pin | Connects to | STM32 / header | Notes |
|---|---|---|---|
| VCC | **+3.3 V** | P2-5 / P2-6 | module runs from 3.3 V; internal charge pump makes the panel voltage |
| GND | **GND** | P1-49 / P1-50 | |
| SCL | **PB10** | P1-? (P1 header) | I2C2_SCL, AF4, open-drain |
| SDA | **PB11** | P1-? (P1 header) | I2C2_SDA, AF4, open-drain |

I2C address: **0x3C** (7-bit) on most modules.

---

## Pull-up resistors

I2C lines idle high through pull-ups:

- **Most breakout modules already include 4.7 kΩ pull-ups** on SCL/SDA — no external parts
  needed.
- The firmware also enables the STM32's **internal pull-ups** (~40 kΩ) so a bare module
  without its own resistors still works at 100 kHz.
- If you later run at 400 kHz on long wires and see flaky ACKs, add external 2.2–4.7 kΩ
  pull-ups to 3.3 V.

---

## Bench bring-up notes

- Logic levels are 3.3 V throughout. Do not feed the OLED 5 V on VCC unless its datasheet
  explicitly allows it (many do, via an onboard regulator — but the DISC1 supplies 3.3 V).
- First test: flash the demo and look for the "GUITAR-CV" banner. If I2C ACKs (visible on
  the Saleae) but the screen is dark, suspect the charge-pump command (`0x8D 0x14`) or VCC.
- Probe SCL = PB10 and SDA = PB11 with the Saleae I2C analyzer; expect address 0x3C followed
  by command (0x00) and data (0x40) control bytes.
