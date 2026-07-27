# Analog Front End — Rev A.1

!!! info "Current design"
    This page describes the **Rev A.1** analog front end: a **3.3 V single-supply,
    TLV9062-based** design. The earlier TL072/TL074 + TLE2426 virtual-ground concept is
    **superseded** — see [ADR-005](../decisions/adr-005-analog-front-end.md) for the
    decision record and rationale.

## Design Summary

The AFE conditions an instrument-level guitar signal for the STM32 ADC using a single
**TLV9062** dual rail-to-rail op-amp running from the **3.3 V analog supply**. One
channel buffers a mid-supply bias voltage (VBIAS ≈ 1.65 V); the other provides a
non-inverting gain stage referenced to VBIAS. The AFE **preserves the waveform** — it
must not intentionally square or clip the signal. All pitch detection, envelope
detection, gate generation, confidence estimation, and quantization happen **in
firmware**, downstream of the ADC.

| Parameter | Value |
|---|---|
| Supply | 3.3 V single-supply (analog rail) |
| Op-amp | TLV9062 (dual, rail-to-rail I/O, specified at 3.3 V) |
| VBIAS | ≈ 1.65 V from 10 kΩ / 10 kΩ divider, buffered by op-amp channel A |
| VBIAS divider filtering | 10 µF ∥ 100 nF |
| Input series protection | 2.2 kΩ |
| RF shunt filter | 220 pF to analog ground |
| AC coupling | 100 nF |
| Post-coupling bias resistor | 1 MΩ to VBIAS (**sets guitar input impedance**) |
| Gain stage | Non-inverting, referenced to VBIAS |
| Rf / Rg | 39 kΩ / 10 kΩ (to VBIAS) |
| Nominal gain | ≈ 4.9× |
| ADC output filter | 3.3 kΩ series + 10 nF to analog ground (first-order, fc ≈ 4.8 kHz) |
| ADC sample rate target | 24 kHz |

## Signal Path

```mermaid
flowchart LR
    J[Guitar jack] --> RP["2.2 kΩ<br/>series protection"]
    RP --> CRF["220 pF<br/>RF shunt"]
    CRF --> CAC["100 nF<br/>AC coupling"]
    CAC --> RB["1 MΩ to VBIAS<br/>bias + input impedance"]
    RB --> A2["TLV9062 ch B<br/>non-inv gain ≈4.9×<br/>ref VBIAS"]
    A2 --> RCF["3.3 kΩ + 10 nF<br/>ADC filter ≈4.8 kHz"]
    RCF --> ADC[STM32 ADC pin]
    VD["3.3 V → 10k/10k divider<br/>10 µF ∥ 100 nF"] --> A1["TLV9062 ch A<br/>VBIAS buffer"]
    A1 -. VBIAS ≈ 1.65 V .-> RB
    A1 -. VBIAS .-> A2
```

## Firmware-Facing Consequences

These are the facts firmware must build against — they replace any earlier assumptions
from the TL072/TLE2426 concept:

- **Silence sits at VBIAS ≈ 1.65 V**, i.e. near ADC midscale — but the midpoint **must
  be measured in firmware at runtime** (silence calibration), never assumed to be
  exactly code 2048. Resistor tolerance, rail variation, and op-amp offset all move it.
- **Signal range:** the guitar waveform is centered on VBIAS and amplified ≈ 4.9×,
  bounded by the rail-to-rail output swing of the TLV9062 (≈ 0 V … 3.3 V).
- **Input impedance** presented to the guitar is the 1 MΩ post-coupling bias resistor.
- **Anti-alias filtering is first-order at ≈ 4.8 kHz** against a 24 kHz sample rate —
  adequate for a fundamental-pitch application (guitar fundamentals ≤ ~1.3 kHz), but
  high harmonics are only attenuated ~20 dB/decade. Firmware should not assume a
  brick-wall-clean spectrum above Nyquist.
- **Both op-amp channels are consumed** (VBIAS buffer + gain stage). Additional analog
  stages need another package.
- **Envelope/onset detection is digital.** There is no analog envelope follower or
  comparator path in Rev A.1.

## Known Limitations

- The 2.2 kΩ series resistor plus the op-amp's internal structures provide only basic
  input protection. **Eurorack mispatching (±12 V into the audio input) is not yet
  robustly protected** — this needs revisiting before the module goes into a rack.
- First-order ADC filter (see above): aliasing of high harmonics is possible; the YIN
  window and 24 kHz rate were chosen with this in mind.
- Gain is fixed at ≈ 4.9×; there is no instrument/line level switch in Rev A.1.

## Superseded Design (for the record)

The original concept used TL072/TL074 op-amps on higher-voltage rails with a TLE2426
rail-splitter virtual ground, leaving open the possibility of analog envelope
detection. It was dropped in favor of the simpler single-supply design; the OPA1678 was
also considered and rejected because it is not specified for 3.3 V operation. Full
rationale: [ADR-005](../decisions/adr-005-analog-front-end.md).

## Still To Do

- Breadboard validation and scope captures
- Measured VBIAS / silence-code data for the calibration module
- Final schematic capture
