# Saleae Logic 2 — SPI Decode Setup (MCP4922)

How to configure a Saleae Logic 2 capture to decode the SPI bus driving the MCP4922 DAC
during bring-up and bench testing. Every setting below matches how the firmware actually
drives the bus — see [SPI Driver](../firmware/spi-driver.md),
[MCP4922 Command Word](../reference/mcp4922-command-word.md),
[Register Map](../reference/register-map.md), and [`src/spi2.c`](../../src/spi2.c)
(`/* CPOL=0, CPHA=0, MSB first */`).

See also: [MCP4922 Wiring](./wiring-mcp4922.md),
[DAC Output — CV Generation](../firmware/dac-output.md).

---

## Probe / channel mapping

A typical 4-channel bench setup:

| Logic 2 channel | Probe to | STM32 pin | SPI analyzer role |
|---|---|---|---|
| **1** | CS̄ | PB12 (P1-36) | **Enable** |
| **2** | SCK | PB13 (P1-37) | **Clock** |
| **3** | SDI | PB15 / MOSI (P1-39) | **MOSI** |
| **4** | VOUTA | MCP4922 pin 14 | **analog** (not part of the SPI analyzer) |

The DAC is write-only, so there is **no MISO** — leave it unassigned.

---

## SPI analyzer settings

Add an **SPI** analyzer and set it exactly as follows:

| Analyzer field | Value | Why |
|---|---|---|
| MOSI | **Channel 3** (SDI) | data into the DAC |
| MISO | **None** | DAC is write-only |
| Clock (SCK) | **Channel 2** | |
| Enable (CS) | **Channel 1** | |
| Significant Bit | **Most Significant Bit First (Standard)** | MSB first |
| Bits per Transfer | **16** | one 16-bit command word per transaction |
| Clock State (CPOL) | **Clock is Low when inactive (CPOL = 0)** | SCK idles low |
| Clock Phase (CPHA) | **Data is Valid on Clock Leading (Rising) Edge (CPHA = 0)** | sample on rising edge |
| Enable line | **Enable line is Active Low** | CS idles high, asserts low |

This is **SPI Mode 0**, MSB-first, 16-bit, active-low CS — exactly the firmware's
`SPI2_CR1` configuration (CPOL=0, CPHA=0, DFF=1, LSBFIRST=0).

### Channel 4 (DAC output) is analog

CH4 is the analog CV output — enable it as an **analog** channel and do **not** add it to
the SPI analyzer. Because LDAC̄ is tied to GND, VOUTA latches the new value on the
**rising edge of CS**, so you should see the analog level step right after each frame ends.

### Logic levels

All signals are **3.3 V** logic. The default digital threshold is fine; don't probe with a
5 V assumption.

---

## Troubleshooting "settings don't match what is being read"

Logic 2 flags idle-state / transition mismatches when a polarity setting is inverted
relative to the bus. Map the symptom to the fix:

| Symptom | Cause | Fix |
|---|---|---|
| Clock idle-state warning; nothing decodes | CPOL set to *Clock is High when inactive* | Set **CPOL = Clock is Low when inactive** (SCK idles low) |
| No frames detected; Enable "transitions" warning | Enable set to *Active High* | Set **Enable line = Active Low** (CS idles high) |
| Bytes split or merged oddly | Bits-per-transfer ≠ 16 | Set **Bits per Transfer = 16** |
| Bits reversed / nonsense values | LSB-first selected | Set **Most Significant Bit First** |
| Data byte shifted by one bit | CPHA on trailing edge | Set **CPHA = leading (rising) edge** |

---

## Expected decode

Each frame is one 16-bit word: `[A̅/B][BUF][G̅A][SH̅DN][D11..D0]`. For normal CV output the
top nibble is `0b0011` (Channel A) or `0b1011` (Channel B).

| Intent | Decoded word | Analog (CH4) after CS↑ |
|---|---|---|
| Channel A, 0 V | `0x3000` | ≈ 0.000 V |
| Channel A, midscale | `0x3800` | ≈ 1.65 V |
| Channel A, C5 (count 1241) | `0x34D9` | ≈ 1.000 V |
| Channel A, full scale | `0x3FFF` | ≈ 3.3 V |

Values per [MCP4922 Command Word](../reference/mcp4922-command-word.md) and
[DAC Output](../firmware/dac-output.md). Send count **1241** and you should decode
`0x34D9` with CH4 stepping to ≈ 1.000 V (one octave above the C4 reference).
