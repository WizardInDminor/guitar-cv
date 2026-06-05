# CV Math — 1V/oct

## The 1V/oct Standard

The 1V/oct (one volt per octave) standard is the control voltage convention used by Eurorack and most analog modular synthesizers. It defines a linear relationship between voltage and pitch where every doubling of frequency corresponds to exactly 1 volt of CV change.

**Reference point (universally adopted):**

```
C4 (middle C, 261.63 Hz) = 0V
```

From there:

| Note | Frequency | CV |
|---|---|---|
| C3 | 130.81 Hz | -1.000V |
| C4 | 261.63 Hz | 0.000V |
| C5 | 523.25 Hz | +1.000V |
| C6 | 1046.50 Hz | +2.000V |

Within each octave, the 12 equal-tempered semitones divide the 1V span equally:

```
1 semitone = 1/12 V ≈ 83.33 mV
```

---

## MIDI Note Numbers

MIDI assigns an integer to every note. C4 = 60. Each semitone is ±1.

The voltage for any MIDI note relative to C4 is:

```
V = (note - 60) / 12.0
```

---

## DAC Resolution

The MCP4922 is a 12-bit DAC with 4096 possible output values (0–4095). With a 3.3V reference:

```
Resolution = 3.3V / 4096 ≈ 0.8056 mV per count
```

One semitone in counts:

```
counts per semitone = (1/12 V) / (3.3V / 4096)
                    = 4096 / (12 × 3.3)
                    ≈ 103.6 counts
```

This gives approximately 103 distinct DAC steps per semitone — far more than needed for accurate pitch output.

---

## The Complete Formula

To convert a MIDI note number to a DAC count:

```
count = (note - ref_note) × (4096 / (12 × Vref))
      = (note - 60) × 103.66
```

In C:

```c
#define VREF               3.3f
#define DAC_COUNTS         4096.0f
#define SEMITONES_PER_VOLT 12.0f
#define REF_NOTE           60      // C4

uint16_t note_to_dac(uint8_t midi_note) {
    float semitones = (float)(midi_note - REF_NOTE);
    float voltage   = semitones / SEMITONES_PER_VOLT;
    float count     = (voltage / VREF) * DAC_COUNTS;

    if (count < 0.0f)    count = 0.0f;
    if (count > 4095.0f) count = 4095.0f;

    return (uint16_t)count;
}
```

---

## Reference Table

| Note | MIDI | Semitones | Voltage | DAC Count |
|---|---|---|---|---|
| C4 (ref) | 60 | 0 | 0.000V | 0 |
| C#4 / Db4 | 61 | 1 | 0.083V | 104 |
| D4 | 62 | 2 | 0.167V | 207 |
| E4 | 64 | 4 | 0.333V | 414 |
| A4 | 69 | 9 | 0.750V | 931 |
| C5 | 72 | 12 | 1.000V | 1241 |
| A4+octave | 81 | 21 | 1.750V | 2172 |
| C6 | 84 | 24 | 2.000V | 2483 |

---

## Calibration Consideration

The formula assumes Vref = exactly 3.3V. In practice the 3.3V rail on the DISC1 and any LDO-based supply will deviate slightly. This means:

- Initial measurements will be close but not exact
- Calibration involves measuring actual Vref with a multimeter and updating the constant
- Alternatively, measure output at two known notes and derive a calibration factor empirically against a reference oscillator

Calibration is deferred to a dedicated session after basic SPI and DAC bring-up is verified.

---

## Why This Works Musically

Equal temperament divides the octave into 12 logarithmically equal steps. The 1V/oct standard maps this logarithmic pitch space onto a linear voltage scale — the synthesizer oscillator's exponential V/oct converter undoes the linearization and produces the correct pitch ratio. The math works out cleanly because the DAC, the convention, and the oscillator's response function are all designed to meet at this interface.
