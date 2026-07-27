# Guitar-to-CV Sequencer Capstone — Project Charter

## Project Title

**Guitar-to-CV Sequencer / Embedded Pitch-to-Control-Voltage Instrument Interface**

---

## 1. Project Purpose

This project is an academic capstone prototype that converts monophonic guitar input into real-time control signals for modular synthesizer systems. The system detects pitch and note events from an instrument-level audio signal, quantizes or preserves the performed notes as desired, and outputs **1V/oct CV**, **gate**, and related timing/control signals suitable for Eurorack-compatible systems.

The project sits at the intersection of **embedded systems**, **real-time signal processing**, **music technology**, and **human-machine interface design**. It is intended to demonstrate practical engineering skill in hardware/software integration while also producing a musically meaningful and extensible prototype.

---

## 2. Project Vision

Create a standalone embedded device that allows a guitar to function as a performance and sequencing controller for modular synthesis systems.

The device should:

* Detect monophonic pitch across the practical guitar range, including low notes
* Detect note onsets / envelope events reliably enough to drive gate behavior
* Convert detected pitch into stable CV output in real time
* Allow captured note events to be stored as sequenced patterns
* Support quantized and unquantized operation
* Provide a simple but usable embedded UI for performance and configuration

The long-term vision is a Eurorack-compatible instrument that feels like a serious prototype with room for future productization.

---

## 3. Problem Statement

Guitar is a rich expressive instrument, but integrating it directly with modular synthesizers in a musically useful embedded form is nontrivial. Existing approaches often involve compromises in latency, tracking stability, usability, or hardware openness.

This project addresses the technical challenge of building a custom embedded system that:

* Accepts real guitar input
* Performs real-time pitch and envelope detection
* Outputs modular-compatible voltage control signals
* Preserves musical usefulness through quantization, timing control, and saved sequences

The project also serves as a platform for learning and demonstrating:

* Bare-metal MCU development
* Real-time DSP / pitch detection tradeoffs
* Mixed-signal hardware design
* Embedded UI architecture
* Firmware modularity and testability

---

## 4. Primary Objectives

1. Build a working prototype that accepts guitar audio and detects monophonic pitch in real time.
2. Generate **1V/oct CV** and **gate** outputs suitable for Eurorack systems.
3. Support **quantized and unquantized** sequence capture/playback behavior.
4. Provide a basic embedded user interface using buttons, a scroll/selection control, and a display.
5. Allow the user to define or tap tempo and use timing information meaningfully.
6. Demonstrate a structured firmware architecture that is modular, testable, and expandable.
7. Produce a capstone-quality prototype and accompanying formal report.

---

## 5. MVP Scope

The MVP is the minimum complete system required for capstone success.

### Core Functional Requirements

* Instrument/line-compatible input path
* Monophonic pitch detection over full intended guitar range
* Envelope or onset detection sufficient to generate gate behavior
* Real-time CV output using **1V/oct** convention
* Gate output for note activity / triggering
* Sequence recording and storage of pitch/gate information
* Ability to store **quantized** sequences
* Ability to store **unquantized** sequences
* Tap tempo or definable tempo control
* Basic UI for mode selection and parameter interaction
* Display feedback for settings / state / sequence information

### Basic Hardware/Platform Requirements

* Microcontroller-based embedded implementation
* External DAC for analog CV generation
* OLED or equivalent compact display
* Physical controls for navigation and interaction
* Eurorack-conscious signal and power design direction

---

## 6. Stretch Goals

These are officially desirable but not required for MVP completion.

### Stretch Features

* **Looper functionality**
* **MIDI output**
* **Loop chaining / pattern chaining**
* Additional trigger generation for drum synths or clocked modular events
* Expanded mode system
* External clock/gate synchronization improvements
* Training / ear-training mode

### Training Mode Concept

A proposed advanced mode is a **training mode** in which the device outputs a target note and gate CV pair, and the user attempts to match the target pitch on their instrument. The system then detects the played note and indicates whether it is correct, sharp, or flat. This could later expand into interval, phrase, and gamified challenge modes.

---

## 7. Technical Constraints and Design Commitments

The following choices are already effectively locked in and define the project boundary.

### Locked Core Components

* **MCU:** STM32F405RG
* **DAC:** MCP4922
* **Display:** SSD1306 OLED
* **Development Style:** Bare-metal C preferred; FreeRTOS optional if justified

### Development Philosophy

* Preference for **full bare-metal C** rather than vendor HAL-heavy abstraction
* Desire to stay close to the hardware for learning and control
* Firmware architecture should remain understandable and extensible
* Avoid unnecessary framework overhead unless it clearly reduces risk

### Final Form Factor Direction

* Final design should move toward **Eurorack-compatible PCB implementation**
* Project should respect likely Eurorack constraints for power, size, and connectivity

---

## 8. System-Level Functional Description

At a high level, the system operates as follows:

1. **Input Stage**

   * Receive guitar or line-level signal
   * Condition signal using analog front-end circuitry
   * Prepare signal for ADC and/or supporting analog detection

2. **Signal Analysis**

   * Estimate pitch in real time
   * Detect note activity, onset, or envelope
   * Determine stable control information suitable for output and sequencing

3. **Control Logic**

   * Quantize pitch if enabled
   * Generate timing-aligned sequence events
   * Support mode-specific behavior
   * Manage recording, playback, tempo, and user settings

4. **Output Stage**

   * Convert pitch data into analog CV via DAC
   * Generate digital or analog-compatible gate/trigger outputs
   * Interface safely and meaningfully with Eurorack gear

5. **User Interface**

   * Present mode/state information on OLED
   * Allow user navigation and parameter editing via buttons/encoder
   * Expose core performance controls without excessive UI complexity

---

## 9. Hardware Direction

### Expected Major Subsystems

* Audio input and analog front-end
* ADC sampling path and/or analog envelope path
* STM32F405 control core
* SPI DAC output stage for CV
* Gate/trigger output circuitry
* OLED display interface (I2C)
* Buttons / encoder input hardware
* Eurorack power conditioning and regulation

### Hardware Design Notes

* User intends to design the **analog front-end** rather than treating it as a black box
* Envelope detection may be analog, digital, or hybrid depending on implementation tradeoffs
* External clock/gate input is desired
* Input path should support selectable instrument/line use if practical

---

## 10. Firmware Direction

The firmware should be designed to support both MVP delivery and future expansion.

### Firmware Goals

* Clean modular architecture
* Real-time responsiveness
* Clear separation of hardware drivers, signal processing, control logic, and UI logic
* Expandability for later features such as looper/MIDI/training mode
* Testability where practical, especially around state/control logic

### Likely Firmware Layers

* Low-level hardware drivers
* Timing/scheduler infrastructure
* ADC / sampling subsystem
* Pitch detection subsystem
* Envelope / onset subsystem
* Sequence engine
* CV/gate output control
* UI/menu/state machine layer
* Persistent settings / save behavior if included

A major project objective is not just to “make it work,” but to organize the firmware so that it remains maintainable as features grow.

---

## 11. Performance Expectations

The prototype should aim for:

* Real-time pitch detection suitable for musical use
* Stable enough note tracking for monophonic guitar performance
* Functional support for low notes, including low B target range if achievable within scope
  (*scope clarification 2026-07-27: low E2 ≈ 82.41 Hz is the required MVP minimum; low B0
  ≈ 30.87 Hz remains a stretch target — see [requirements](requirements.md) for the
  normative statement*)
* Gate behavior that feels musically coherent
* CV output stable enough to drive external oscillators predictably
* Latency low enough that the system feels like an instrument rather than an offline processor

These are engineering targets rather than guarantees of perfect tracking under all playing conditions.

---

## 12. Success Criteria

The project will be considered successful if it demonstrates the following:

### Minimum Success

* Guitar input is received and processed by the system
* Monophonic pitch is detected in real time
* Corresponding CV and gate are generated
* Quantized and unquantized note/sequence handling is demonstrated
* User can interact with the system through a basic UI
* Prototype can be shown live using guitar and modular gear

### Strong Success

* Tracking is stable enough for convincing demonstration
* Architecture is clean and clearly documented
* Hardware and firmware decisions are well justified
* Final report explains design tradeoffs, implementation, limitations, and future work

---

## 13. Deliverables

### Required Deliverables

* Working prototype
* Formal capstone report
* Demonstration of live functionality

### Likely Supporting Deliverables

* Firmware source code
* Hardware schematics / design notes
* Architecture documentation
* Test notes / validation observations
* BOM and subsystem breakdowns

---

## 14. Risks and Technical Challenges

Key project risks include:

### Signal Processing Risks

* Pitch detection latency vs stability tradeoff
* False triggering or unstable tracking during transient-rich playing
* Difficulty tracking low-frequency guitar notes cleanly
* Sensitivity to input dynamics, picking style, or noise

### Embedded Risks

* Real-time scheduling pressure on MCU resources
* Interaction between DSP workload, UI updates, and output timing
* Calibration requirements for CV accuracy
* DAC/output conditioning issues affecting pitch stability

### Hardware Risks

* Analog front-end complexity
* Noise and grounding issues in mixed-signal design
* Eurorack interfacing and protection considerations
* Power design and output scaling accuracy

### Scope Risks

* Stretch goals could consume time needed for MVP polish
* Firmware architecture could become overbuilt if not kept pragmatic
* Custom hardware work may introduce schedule compression late in the project

---

## 15. Risk Management Approach

To control project risk, the implementation should proceed in stages:

1. Prove core signal path and basic pitch detection
2. Prove CV/gate output independently
3. Integrate signal analysis with output control
4. Add sequence engine and timing behavior
5. Add UI and mode control
6. Reserve stretch goals until MVP is stable

This staged approach protects the capstone outcome by ensuring the core musical/control loop works before feature expansion.

---

## 16. Development Strategy

A practical development progression for this project is:

### Phase 1 — Core Bring-Up

* MCU setup
* Toolchain/debug environment
* DAC bring-up
* OLED bring-up
* Control input bring-up
* Basic timing infrastructure

### Phase 2 — Audio and Detection Foundations

* Analog input chain validation
* ADC data capture
* Envelope/onset experiments
* Pitch detection experiments and algorithm selection

### Phase 3 — Musical Control Path

* CV output mapping and calibration
* Gate generation
* Quantization logic
* Sequence data representation

### Phase 4 — Product-Like Integration

* UI/menu behavior
* Save/load behavior if included
* Performance tuning
* Demo preparation

### Phase 5 — Stretch Features

* MIDI, looper, chaining, training mode, etc.

---

## 17. Available Resources

User has access to:

* Oscilloscope
* Logic analyzer
* Modular synth gear
* Audio tools for testing
* STM32 development hardware
* Embedded C development experience and interest in hardware-near design

These resources materially reduce project risk in debugging mixed-signal and embedded timing issues.

---

## 18. Stakeholders

### Primary Stakeholder

* **Project Owner / Developer:** Matt

### Academic Stakeholder

* **Capstone Advisor / Evaluator**

### End-Use Context

* Guitar/modular performance and experimentation
* Academic demonstration of engineering capability
* Potential future portfolio/commercial inspiration

---

## 19. Out of Scope for MVP

Unless needed to rescue core functionality, the following are not required for MVP:

* Polyphonic pitch detection
* Full DAW-style sequencing environment
* Complex graphical UI
* Large preset management system
* Commercial-grade enclosure/industrial design polish
* Full product manufacturing readiness

These may inform future work, but they should not displace the core capstone goals.

---

## 20. Charter Summary

This capstone project is a real-time embedded guitar-to-CV sequencer system built around the STM32F405RG. Its purpose is to transform monophonic guitar input into useful modular synthesizer control signals while demonstrating competence in embedded C development, signal processing, hardware/software integration, and system architecture.

The MVP centers on reliable pitch and gate extraction, CV output, sequencing capability, quantized/unquantized storage, and a practical embedded UI. Stretch features such as MIDI, looping, chaining, and training mode remain desirable but secondary. The guiding principle is to deliver a musically meaningful, technically credible prototype with clean architecture and a strong live demonstration.

---

## 21. Immediate Re-Entry Checklist

To resume work efficiently after time away, start by answering these questions:

1. What hardware is already physically in hand and verified?
2. What development environment is already working?
3. What firmware bring-up pieces are already complete?
4. What is the current status of pitch detection exploration?
5. What is the current status of DAC/CV output testing?
6. What is the current status of UI bring-up?
7. What is the next smallest demonstrable milestone?

A good first milestone after re-entry is:
**“Bring up the MCU, display, DAC, and one basic end-to-end control path that proves the platform is alive.”**

