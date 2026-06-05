#ifndef CV_H
#define CV_H

#include <stdint.h>

/*
 * 1V/oct CV mapping for the MCP4922 (12-bit) DAC.
 *
 * Reference: C4 (MIDI 60) = 0V. Each semitone is 1/12 V. With a Vref of 3.3V
 * and gain 1x, full-scale (count 4095) corresponds to ~3.3V.
 *
 * CV_VREF is the one value to adjust during calibration once the actual DISC1
 * 3.3V rail has been measured.
 */
#define CV_VREF        3.3f    /* DAC reference voltage (volts)              */
#define CV_DAC_COUNTS  4096.0f /* 12-bit DAC range                          */
#define CV_REF_NOTE    60      /* MIDI note mapped to 0V (C4)               */
#define CV_SEMIS_OCT   12.0f   /* semitones per octave / per volt           */
#define CV_DAC_MAX     4095u   /* max valid DAC count                        */

/*
 * Convert a MIDI note number to a DAC count using the 1V/oct convention.
 * Uses round-to-nearest and clamps to [0, CV_DAC_MAX]. Pure function: no
 * hardware access, safe to unit-test on the host.
 */
uint16_t note_to_dac(uint8_t midi_note);

#endif /* CV_H */
