#include <stdint.h>

#include "cv.h"
#include "dac.h"
#include "mcp4922.h"
#include "i2c1.h"
#include "ssd1306.h"
#include "systick.h"

/* HSI default out of reset; becomes 168000000u after PLL bring-up. */
#define SYSTEM_CORE_CLOCK_HZ 16000000u

/* Onboard LED heartbeat (PD12, green LD4 on the DISC1). */
#define RCC_AHB1ENR (*(volatile uint32_t *)0x40023830)
#define GPIOD_MODER (*(volatile uint32_t *)0x40020C00)
#define GPIOD_ODR   (*(volatile uint32_t *)0x40020C14)
#define LED_PIN     12

int main(void)
{
    systick_init(SYSTEM_CORE_CLOCK_HZ);

    /* Heartbeat LED. */
    RCC_AHB1ENR |= (1u << 3);                 /* GPIOD clock         */
    GPIOD_MODER &= ~(0x3u << (LED_PIN * 2));
    GPIOD_MODER |=  (0x1u << (LED_PIN * 2));  /* PD12 output         */

    dac_init();

    /* OLED bring-up: white screen confirms I2C + SSD1306 init. */
    i2c1_init();
    delay_ms(25);    /* let SSD1306 VCC stabilize before first command */
    ssd1306_init();
    ssd1306_fill(0xFF);

    /*
     * Bench bring-up: walk the documented first-test points through the full
     * path (note -> count -> command word -> SPI). VOUTA (pin 14) should read
     * ~0 V, ~1.000 V (C5), ~2.000 V (C6). The DAC is re-written each step so
     * the SPI bursts are repeatable/triggerable on a scope.
     *
     * Scheduled with the non-blocking time_elapsed() pattern — the loop stays
     * free for future work (UI, sequencer) between steps.
     */
    static const uint8_t notes[] = { 60, 72, 84 };  /* C4, C5, C6 */
    unsigned idx = 0;
    uint32_t last_step = millis();

    dac_write(MCP4922_CHANNEL_A, note_to_dac(notes[idx]));

    while (1) {
        if (time_elapsed(last_step, 500u)) {
            last_step += 500u;                      /* drift-free cadence */
            idx = (idx + 1u) % (sizeof(notes) / sizeof(notes[0]));
            dac_write(MCP4922_CHANNEL_A, note_to_dac(notes[idx]));
            GPIOD_ODR ^= (1u << LED_PIN);           /* heartbeat */
        }
    }
}
