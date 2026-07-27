#ifndef I2C1_H
#define I2C1_H

#include <stdint.h>

/*
 * Minimal polling I2C1 master driver (write-only).
 *
 * I2C1 on PB6 (SCL, AF4) / PB7 (SDA, AF4); open-drain, internal pull-ups.
 * Standard mode timing (CR2.FREQ, CCR, TRISE) is computed from the supplied
 * APB1 clock at init. Used to drive the SSD1306 OLED. See
 * docs/firmware/i2c-driver.md and docs/reference/register-map.md.
 *
 * Note: external 4.7 kΩ pull-ups are preferred over the weak internal ones;
 * use internal pull-ups only for initial bring-up on a short wire.
 */

typedef enum {
    I2C_OK = 0,
    I2C_ERR_INVALID_CONFIG,  /* pclk/scl outside standard-mode field limits */
} i2c_status_t;

/*
 * Configure GPIO + I2C1 from the actual APB1 clock and enable the
 * peripheral. scl_hz is the target SCL rate, standard mode only
 * (<= 100 kHz). Call once at startup.
 */
i2c_status_t i2c1_init(uint32_t pclk1_hz, uint32_t scl_hz);

/*
 * Transmit len bytes from buf to the 7-bit address addr7.
 * Generates: START → addr+W → buf[0..len-1] → STOP.
 */
void i2c1_write(uint8_t addr7, const uint8_t *buf, uint16_t len);

/*
 * Pure timing-math helpers — no hardware access, unit-tested on the host
 * (test/test_clock.c). Standard mode formulas per RM0090:
 *
 *   FREQ  = pclk / 1 MHz                   (integer, field limits 2..50)
 *   CCR   = ceil(pclk / (2 × f_SCL))       (ceil keeps SCL <= target; min 4)
 *   TRISE = FREQ + 1                       (1000 ns max rise time; 6-bit field)
 */
static inline uint32_t i2c_freq_field(uint32_t pclk_hz)
{
    return pclk_hz / 1000000u;
}

static inline uint32_t i2c_sm_ccr(uint32_t pclk_hz, uint32_t scl_hz)
{
    return (pclk_hz + (2u * scl_hz) - 1u) / (2u * scl_hz);
}

static inline uint32_t i2c_sm_trise(uint32_t pclk_hz)
{
    return i2c_freq_field(pclk_hz) + 1u;
}

#endif /* I2C1_H */
