# Firmware Architecture

## Design Philosophy

The firmware is bare-metal C targeting the STM32F407/F405. No HAL, no CubeIDE, no vendor-generated code. Every peripheral is configured at the register level with explicit reasoning for each decision.

The guiding principle is clean separation of concerns across layers — hardware drivers do not contain business logic, and the control layer does not reach into hardware registers directly.

---

## Firmware Layers

```
┌─────────────────────────────────────┐
│         UI / State Machine          │  Menu, mode, display output
├─────────────────────────────────────┤
│         Control Logic               │  Sequence engine, quantization,
│                                     │  tempo, mode behavior
├─────────────────────────────────────┤
│         Signal Processing           │  Pitch detection (YIN),
│                                     │  envelope / onset detection
├─────────────────────────────────────┤
│         Hardware Drivers            │  SPI, I2C, ADC, GPIO, timers
├─────────────────────────────────────┤
│         Startup / Platform          │  Reset handler, clock config,
│                                     │  vector table, linker script
└─────────────────────────────────────┘
```

---

## Source Structure

```
~/dev/school/stm32/guitar-cv/
├── src/
│   ├── main.c              # Entry point + bring-up demo
│   ├── spi2.c              # SPI2 peripheral driver (MCP4922)
│   ├── mcp4922.c           # MCP4922 command word packing
│   ├── dac.c               # DAC write abstraction
│   ├── cv.c                # note_to_dac() — 1V/oct conversion
│   ├── i2c1.c              # I2C1 peripheral driver (SSD1306)
│   └── ssd1306.c           # SSD1306 OLED device driver
├── include/                # Matching headers for each src/ module
├── test/
│   └── test_cv.c           # Host-side unit tests (cv.c + mcp4922.c)
├── startup/
│   └── startup_stm32f407.s # Reset handler, vector table
├── ld/
│   └── stm32f407.ld        # Linker script
├── build/                  # Compiled output (gitignored)
└── Makefile
```

---

## Key Platform Facts

| Parameter | Value |
|---|---|
| MCU (dev) | STM32F407VGT6 |
| MCU (final) | STM32F405RG |
| Flash origin | 0x08000000 |
| SRAM origin | 0x20000000 |
| SRAM size | 128KB |
| Stack top | 0x20020000 |
| FPU | Enabled at start of Reset_Handler |
| Development style | Bare-metal C, register-level |

---

## Subsystem Status

| Subsystem | Status |
|---|---|
| Startup / vector table | ✅ Complete (Session 01) |
| GPIO | ✅ Complete (Session 01) |
| SPI driver (MCP4922) | ✅ Code written (`src/spi2.c`, `src/dac.c`) + hardware-verified 2026-06-05 (Saleae Logic 2 MSO, 0/1/2 V) |
| CV output mapping (note→count) | ✅ Implemented + unit-tested (`src/cv.c`, `src/mcp4922.c`) |
| I2C driver (SSD1306) | ✅ Code written (`src/i2c1.c`, `src/ssd1306.c`) + hardware-verified 2026-06-12 (Saleae Logic 2, white screen confirmed) |
| Timer / SysTick | Not started |
| ADC | Not started |
| Pitch detection | Not started |
| Envelope detection | Not started |
| Gate output | Not started |
| Sequence engine | Not started |
| UI / state machine | Not started |
