#include <stdint.h>

#include "cv.h"
#include "dac.h"
#include "mcp4922.h"
#include "i2c1.h"
#include "ssd1306.h"

/* Onboard LED heartbeat (PD12, green LD4 on the DISC1). */
#define RCC_AHB1ENR (*(volatile uint32_t *)0x40023830)
#define GPIOD_MODER (*(volatile uint32_t *)0x40020C00)
#define GPIOD_ODR   (*(volatile uint32_t *)0x40020C14)
#define LED_PIN     12

static void delay(volatile uint32_t count)
{
    while (count--) { }
}

int main(void)
{
    /* Heartbeat LED. */
    RCC_AHB1ENR |= (1u << 3);                 /* GPIOD clock         */
    GPIOD_MODER &= ~(0x3u << (LED_PIN * 2));
    GPIOD_MODER |=  (0x1u << (LED_PIN * 2));  /* PD12 output         */

    dac_init();

    /* OLED bring-up: white screen confirms I2C + SSD1306 init. */
    i2c1_init();
    delay(200000);   /* ~25 ms: let SSD1306 VCC stabilize before first command */
    ssd1306_init();
    ssd1306_fill(0xFF);

    /*
     * Bench bring-up: walk the documented first-test points through the full
     * path (note -> count -> command word -> SPI). VOUTA (pin 14) should read
     * ~0 V, ~1.000 V (C5), ~2.000 V (C6). The DAC is re-written each step so
     * the SPI bursts are repeatable/triggerable on a scope.
     */
    static const uint8_t notes[] = { 60, 72, 84 };  /* C4, C5, C6 */

    while (1) {
        for (unsigned i = 0; i < sizeof(notes) / sizeof(notes[0]); i++) {
            dac_write(MCP4922_CHANNEL_A, note_to_dac(notes[i]));
            GPIOD_ODR ^= (1u << LED_PIN);           /* heartbeat */
            delay(800000);
        }
    }
}
