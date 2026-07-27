# ADR-005 — Use a 3.3 V TLV9062-Based Analog Front End

**Date:** 2026-07-27  
**Status:** Accepted  
**Supersedes:** The pre-ADR TL072/TL074 + TLE2426 analog front-end concept (charter §9, early hardware pages)  
**Superseded by:** —

---

## Context

The guitar input must be conditioned for the STM32 ADC: gained up from instrument
level, DC-biased into the ADC's unipolar input range, band-limited, and protected. The
original concept (recorded in the charter and early hardware pages) assumed TL072/TL074
op-amps running on higher-voltage rails with a TLE2426 rail-splitter generating a
virtual ground, and left open the option of doing envelope detection in analog
hardware.

Forces at play:

- The ADC input range is 0–3.3 V; whatever the analog domain does, the signal must end
  up biased inside that window.
- Higher-voltage analog rails mean extra regulation, level shifting back down to the
  ADC window, and more power-supply design before the first note can be sampled.
- TL072/TL074 are not rail-to-rail and are not characterized at 3.3 V single-supply;
  their usable swing on a 3.3 V rail would be a fraction of the window.
- The project philosophy is understanding over speed, but also staged risk reduction:
  Phase 2 (pitch detection) is the highest-risk phase and is blocked on a working AFE.
- The MCU has ample compute for digital envelope/onset detection, and firmware DSP is
  host-testable — analog detection hardware is not.

## Decision

Build the Rev A.1 front end as a **3.3 V single-supply design around one TLV9062 dual
rail-to-rail op-amp**: channel A buffers a 10 kΩ/10 kΩ mid-supply divider to produce
VBIAS ≈ 1.65 V; channel B is a non-inverting gain stage (Rf = 39 kΩ, Rg = 10 kΩ to
VBIAS, gain ≈ 4.9×) referenced to VBIAS. Input path: 2.2 kΩ series protection → 220 pF
RF shunt → 100 nF AC coupling → 1 MΩ bias resistor to VBIAS (sets input impedance) →
gain stage → 3.3 kΩ/10 nF ADC filter. All pitch, envelope, gate, and confidence
processing is done in firmware.

## Options Considered

### Option A — TL072/TL074 + TLE2426 virtual ground (original concept)
Classic audio op-amps on wider rails, TLE2426 splitting the rail for a mid-supply
reference, possible analog envelope path.

**Pros:**

- Familiar, forgiving audio parts; parts already on hand
- Headroom for large input transients
- Leaves an analog envelope-detection option open

**Cons:**

- Needs a higher-voltage analog supply and level translation back into the 0–3.3 V ADC
  window — significant extra power and interface design before Phase 2 can start
- TL07x is not rail-to-rail and not specified for 3.3 V operation
- More parts, more supply domains, more grounding/noise surface area
- Analog envelope hardware duplicates what firmware must do anyway for sequencing

### Option B — OPA1678 single-supply
Low-noise audio op-amp in the same topology as the chosen design.

**Pros:**

- Excellent audio noise/distortion specs

**Cons:**

- **Not specified for a 3.3 V supply** (minimum supply is above the available analog
  rail) — disqualifying for this topology

### Option C — TLV9062 3.3 V single-supply (chosen)
Rail-to-rail in/out op-amp specified down to 1.8 V; one package covers VBIAS buffering
and the gain stage.

**Pros:**

- Runs directly from the existing 3.3 V analog rail — no extra supply domain, no
  TLE2426, no level shifting; output range inherently matches the ADC window
- Rail-to-rail output uses nearly the full ADC range at ≈ 4.9× gain
- Two channels = exactly the two functions needed (bias buffer + gain)
- Simplest possible path to a working Phase 2 signal chain

**Cons:**

- Less input-overload headroom than a wide-rail design — protection relies on the
  2.2 kΩ series resistor and op-amp clamping (revisit before Eurorack deployment)
- General-purpose (not audio-specialty) noise specs — acceptable for pitch/envelope
  extraction, which is not a hi-fi path

## Rationale

- **Why bias to VBIAS:** the ADC is unipolar (0–3.3 V) while a guitar signal is
  bipolar around 0 V. Centering the waveform on a buffered mid-supply VBIAS puts
  silence at ≈ 1.65 V (near ADC midscale) and lets the full swing fit the window.
- **Why silence is midscale in hardware but calibrated in firmware:** resistor
  tolerance in the divider, rail variation, and op-amp offset make the true silent-input
  code drift from exactly 2048. Firmware must measure the midpoint at runtime
  (silence calibration) and center samples on the measured value, never on a constant.
- **Why envelope and pitch detection stay digital:** firmware already needs
  note/level data for sequencing; digital detection is host-testable, tunable without
  soldering, and removes an entire analog subsystem. The MCU has the headroom.
- **Why 1 MΩ input impedance:** a passive guitar pickup needs ≥ ~1 MΩ to avoid tone
  loading; the post-coupling bias resistor doubles as the input impedance definition.
- **Why gain ≈ 4.9:** brings typical instrument-level signals (~100 mVpp–1 Vpp)
  into a healthy fraction of the 3.3 V window without clipping normal playing.
- **Why the waveform must be preserved:** YIN operates on the sampled waveform;
  squaring/clipping in hardware would destroy the information the detector needs.
  Any zero-cross-style simplification is a firmware choice, not a hardware commitment.
- **Known accepted limitation — first-order 4.8 kHz ADC filter:** at 24 kS/s the
  filter attenuates but does not eliminate energy above Nyquist. Guitar fundamentals
  top out around 1.3 kHz, so residual aliased harmonics are treated as tolerable
  noise for pitch estimation. If it proves problematic, a second RC or a higher-order
  stage is the escalation path.
- **Known accepted limitation — input protection:** the 2.2 kΩ series resistor is
  adequate for instrument signals but not a guarantee against Eurorack mispatching
  (±12 V patched into the audio input). This is explicitly deferred, to be resolved
  before the module lives in a rack.

## Consequences

- Phase 2 can start against a fixed, simple analog contract: signal centered on a
  measured VBIAS, ≈ 4.9× gain, 24 kHz sampling, first-order-filtered input.
- Power design simplifies: the AFE needs only the clean 3.3 V analog rail; the ±12 V
  Eurorack entry feeds regulation rather than the op-amps directly.
- Both TLV9062 channels are used; any future analog stage (e.g. line-level pad,
  stronger anti-aliasing) needs another package or a Rev B.
- The firmware roadmap gains explicit modules: silence/midpoint calibration, sample
  centering, digital envelope + gate hysteresis.
- TL072/TL074, TLE2426 (and the OPA1678 candidate) drop out of the design; BOM stock
  of those parts is spare/bench inventory, not project material.

## References

- [Analog Front End — Rev A.1](../hardware/analog-front-end.md)
- [Power Design](../hardware/power.md)
- TI TLV9062 datasheet (supply range, rail-to-rail I/O)
- TI OPA1678 datasheet (minimum supply — disqualifying at 3.3 V)
- Related: [ADR-001 SPI Peripheral](adr-001-spi-peripheral.md) (same
  "simplest path that preserves understanding" philosophy)
