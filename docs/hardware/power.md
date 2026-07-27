# Power Design

!!! note "In Definition"
    Detailed power design has not started, but the analog architecture decision
    ([ADR-005](../decisions/adr-005-analog-front-end.md)) fixes the key constraint:
    **the analog front end runs from a single 3.3 V rail.** No higher-voltage analog
    supply and no TLE2426 rail splitter are required.

## Architecture Direction

- **Eurorack entry:** +12 V / −12 V / +5 V bus connector (final module form factor).
  The ±12 V rails power nothing directly in Rev A.1 — they feed regulation.
- **Digital 3.3 V:** LDO regulation for the MCU, DAC, and OLED.
- **Analog 3.3 V:** the AFE (TLV9062, VBIAS divider) runs from a filtered/separated
  3.3 V analog rail. Separation strategy (ferrite + local decoupling vs. dedicated
  LDO) is an open decision.
- **VBIAS (≈ 1.65 V)** is generated inside the AFE from the analog 3.3 V rail
  (10 kΩ/10 kΩ divider, 10 µF ∥ 100 nF, op-amp buffered) — it is a signal reference,
  not a power rail.

## Open Items

- Analog/digital 3.3 V separation approach and grounding plan
- LDO selection and dropout/thermal budget from +12 V
- Power sequencing considerations
- Reverse/mispatch protection at the Eurorack connector
- CV/gate output stage supply needs (output range > 3.3 V would need the ±12 V rails
  and is a separate decision from the AFE)
