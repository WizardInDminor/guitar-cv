# MCP4922 Command Word

## Overview

Every SPI transaction to the MCP4922 is exactly **16 bits**. There are no multi-word transactions and no read operations — the device is write-only from the MCU's perspective.

The 16-bit word consists of 4 configuration bits followed by 12 data bits, sent MSB first.

---

## Bit Field Map

```
Bit:  15    14    13    12    11  10  9   8   7   6   5   4   3   2   1   0
      ────  ────  ────  ────  ──────────────────────────────────────────────
      /A·B  BUF   /GA   /SHDN D11 D10 D9  D8  D7  D6  D5  D4  D3  D2  D1  D0
```

| Bit | Name | Value | Meaning |
|---|---|---|---|
| 15 | /A·B | 0 | Select channel A |
| 15 | /A·B | 1 | Select channel B |
| 14 | BUF | 0 | Unbuffered (use this) |
| 14 | BUF | 1 | Buffered Vref input |
| 13 | /GA | 0 | 2× gain (Vout = 2 × Vref × D/4096) |
| 13 | /GA | 1 | 1× gain (Vout = Vref × D/4096) |
| 12 | /SHDN | 0 | Shutdown channel |
| 12 | /SHDN | 1 | Active — normal operation |
| 11:0 | D[11:0] | 0–4095 | 12-bit output value, MSB first |

---

## Standard Configuration

For CV output at 1× gain, active, unbuffered:

```
Bit 15: channel (0=A, 1=B)
Bit 14: 0  (unbuffered)
Bit 13: 1  (1× gain)
Bit 12: 1  (active)
Bits 11:0: data
```

This means bits 14, 13, 12 = 0b011 for every normal write.

### Channel A, midscale example:

```
Binary:  0 0 1 1  1000 0000 0000
Hex:     0x3800
```

---

## Command Word Builder

```c
uint16_t mcp4922_build(uint8_t channel, uint16_t value) {
    uint16_t cmd = 0;
    cmd |= (channel & 0x1) << 15;  // channel: 0=A, 1=B
    cmd |= (1 << 13);               // GA: 1x gain
    cmd |= (1 << 12);               // SHDN: active
    cmd |= (value & 0x0FFF);        // 12-bit data, mask for safety
    return cmd;
}
```

---

## Timing Requirements

From MCP4922 datasheet Figure 5.1:

| Parameter | Requirement |
|---|---|
| CS idle state | High |
| Clock idle state | Low (CPOL=0) |
| Data sampled on | Rising edge of SCK (CPHA=0) |
| SPI mode | Mode 0 |
| Maximum SCK | 20 MHz |
| CS must be high before next transaction | Yes |

LDAC behavior:

| LDAC state | Output update timing |
|---|---|
| Tied low (our config) | Output updates on rising edge of CS |
| Pulsed | Output updates on falling edge of LDAC pulse |

---

## CS and LDAC Wiring

For this project:

- **CS** — driven by PB12, GPIO output, active low
- **LDAC** — tied permanently to GND

Tying LDAC low means the output updates immediately when CS goes high at the end of each transaction. This is correct for single-channel real-time CV output. If both DAC channels needed to update simultaneously, LDAC would be used as a strobe — not needed for this use case.

---

## Example Transactions

| Intent | channel | value | cmd word |
|---|---|---|---|
| Channel A, 0V (zero) | 0 | 0x000 | 0x3000 |
| Channel A, midscale | 0 | 0x800 | 0x3800 |
| Channel A, full scale | 0 | 0xFFF | 0x3FFF |
| Channel B, midscale | 1 | 0x800 | 0xB800 |
| Channel A, C5 (1V) | 0 | 0x4D9 (1241) | 0x34D9 |
