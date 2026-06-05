/*
 * Host-side unit tests for the pure DAC-conversion logic.
 *
 * These exercise note_to_dac() and mcp4922_command() with no hardware: build
 * and run on the host with `make test`. The SPI transmit path (spi2_write16)
 * is register I/O and is verified separately on the bench, not here.
 */
#include <stdio.h>
#include <stdint.h>

#include "cv.h"
#include "mcp4922.h"

static int failures = 0;
static int checks   = 0;

#define CHECK_EQ(actual, expected, label)                                     \
    do {                                                                      \
        unsigned long _a = (unsigned long)(actual);                           \
        unsigned long _e = (unsigned long)(expected);                         \
        checks++;                                                             \
        if (_a != _e) {                                                       \
            failures++;                                                       \
            printf("FAIL %-28s got %lu, expected %lu\n", (label), _a, _e);    \
        }                                                                     \
    } while (0)

static void test_note_to_dac(void)
{
    /* Round-to-nearest, C4 (MIDI 60) = 0V, Vref = 3.3V. */
    CHECK_EQ(note_to_dac(60),    0, "note_to_dac C4");
    CHECK_EQ(note_to_dac(61),  103, "note_to_dac C#4");
    CHECK_EQ(note_to_dac(62),  207, "note_to_dac D4");
    CHECK_EQ(note_to_dac(64),  414, "note_to_dac E4");
    CHECK_EQ(note_to_dac(69),  931, "note_to_dac A4");
    CHECK_EQ(note_to_dac(72), 1241, "note_to_dac C5");
    CHECK_EQ(note_to_dac(81), 2172, "note_to_dac A5");
    CHECK_EQ(note_to_dac(84), 2482, "note_to_dac C6");
    CHECK_EQ(note_to_dac(96), 3724, "note_to_dac C7");

    /* Below C4 -> negative count -> clamp to 0. */
    CHECK_EQ(note_to_dac(59),    0, "note_to_dac B3 clamp");
    CHECK_EQ(note_to_dac(36),    0, "note_to_dac C2 clamp");
    CHECK_EQ(note_to_dac(0),     0, "note_to_dac 0 clamp");

    /* Above full scale -> clamp to 4095. */
    CHECK_EQ(note_to_dac(108), 4095, "note_to_dac C8 clamp");
    CHECK_EQ(note_to_dac(127), 4095, "note_to_dac 127 clamp");
}

static void test_mcp4922_command(void)
{
    /* Standard config bits: BUF=0, GA=1, SHDN=1 -> 0b0011 in the top nibble. */
    CHECK_EQ(mcp4922_command(MCP4922_CHANNEL_A,    0), 0x3000, "cmd A zero");
    CHECK_EQ(mcp4922_command(MCP4922_CHANNEL_A, 2048), 0x3800, "cmd A midscale");
    CHECK_EQ(mcp4922_command(MCP4922_CHANNEL_A, 4095), 0x3FFF, "cmd A fullscale");
    CHECK_EQ(mcp4922_command(MCP4922_CHANNEL_B,    0), 0xB000, "cmd B zero");
    CHECK_EQ(mcp4922_command(MCP4922_CHANNEL_B, 4095), 0xBFFF, "cmd B fullscale");

    /* Data is masked to 12 bits; channel is masked to 1 bit. */
    CHECK_EQ(mcp4922_command(MCP4922_CHANNEL_A, 0x1234), 0x3234, "cmd data mask");
    CHECK_EQ(mcp4922_command(2,                    0), 0x3000, "cmd chan mask");
}

int main(void)
{
    test_note_to_dac();
    test_mcp4922_command();

    if (failures == 0) {
        printf("OK: %d checks passed\n", checks);
        return 0;
    }
    printf("FAILED: %d of %d checks failed\n", failures, checks);
    return 1;
}
