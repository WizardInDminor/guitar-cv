#include "cv.h"

uint16_t note_to_dac(uint8_t midi_note)
{
    float semitones = (float)((int)midi_note - CV_REF_NOTE);
    float voltage   = semitones / CV_SEMIS_OCT;
    float count     = (voltage / CV_VREF) * CV_DAC_COUNTS;

    if (count < 0.0f) {
        return 0;
    }

    count += 0.5f;                       /* round to nearest */

    if (count > (float)CV_DAC_MAX) {
        return CV_DAC_MAX;
    }

    return (uint16_t)count;
}
