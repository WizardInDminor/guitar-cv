/*
 * I2C2 master driver (polling) — see include/i2c.h.
 *
 * Register-level, no HAL, matching the style of src/spi2.c: absolute register
 * addresses as inline volatile pointers, busy-wait on status flags. The only
 * addition over the SPI driver is a bounded timeout on every wait so a missing
 * or miswired OLED leaves the MCU running (heartbeat keeps blinking) instead of
 * hanging forever on a flag that never sets.
 */
#include "i2c.h"

/* RCC clock enables. */
#define RCC_AHB1ENR (*(volatile uint32_t *)0x40023830) /* bit 1  GPIOBEN */
#define RCC_APB1ENR (*(volatile uint32_t *)0x40023840) /* bit 22 I2C2EN  */

/* GPIOB (PB10 = SCL, PB11 = SDA). */
#define GPIOB_MODER   (*(volatile uint32_t *)0x40020400)
#define GPIOB_OTYPER  (*(volatile uint32_t *)0x40020404)
#define GPIOB_OSPEEDR (*(volatile uint32_t *)0x40020408)
#define GPIOB_PUPDR   (*(volatile uint32_t *)0x4002040C)
#define GPIOB_AFRH    (*(volatile uint32_t *)0x40020424) /* pins 8..15 */

/* I2C2 (base 0x40005800). */
#define I2C2_CR1   (*(volatile uint32_t *)0x40005800)
#define I2C2_CR2   (*(volatile uint32_t *)0x40005804)
#define I2C2_DR    (*(volatile uint32_t *)0x40005810)
#define I2C2_SR1   (*(volatile uint32_t *)0x40005814)
#define I2C2_SR2   (*(volatile uint32_t *)0x40005818)
#define I2C2_CCR   (*(volatile uint32_t *)0x4000581C)
#define I2C2_TRISE (*(volatile uint32_t *)0x40005820)

/* CR1 bits. */
#define I2C_CR1_PE    (1u << 0)
#define I2C_CR1_START (1u << 8)
#define I2C_CR1_STOP  (1u << 9)
#define I2C_CR1_SWRST (1u << 15)

/* SR1 bits. */
#define I2C_SR1_SB   (1u << 0)  /* start condition generated */
#define I2C_SR1_ADDR (1u << 1)  /* address sent / matched    */
#define I2C_SR1_BTF  (1u << 2)  /* byte transfer finished    */
#define I2C_SR1_TXE  (1u << 7)  /* data register empty       */
#define I2C_SR1_AF   (1u << 10) /* acknowledge failure (NACK)*/

/* SR2 bits. */
#define I2C_SR2_BUSY (1u << 1)

#define SCL_PIN 10  /* PB10 — I2C2_SCL (AF4) */
#define SDA_PIN 11  /* PB11 — I2C2_SDA (AF4) */

/* Spin budget for any single flag wait. Generous vs a 100 kHz byte (~90 us). */
#define I2C_TIMEOUT 100000u

void i2c2_init(void)
{
    /* 1. Enable GPIOB and I2C2 clocks. */
    RCC_AHB1ENR |= (1u << 1);
    RCC_APB1ENR |= (1u << 22);

    /* 2. MODER: PB10/PB11 alternate function (10). */
    GPIOB_MODER &= ~((0x3u << (SCL_PIN * 2)) | (0x3u << (SDA_PIN * 2)));
    GPIOB_MODER |=  ((0x2u << (SCL_PIN * 2)) | (0x2u << (SDA_PIN * 2)));

    /* 3. OTYPER: open-drain (1) — mandatory for I2C. */
    GPIOB_OTYPER |= (1u << SCL_PIN) | (1u << SDA_PIN);

    /* 4. OSPEEDR: high speed (10). */
    GPIOB_OSPEEDR &= ~((0x3u << (SCL_PIN * 2)) | (0x3u << (SDA_PIN * 2)));
    GPIOB_OSPEEDR |=  ((0x2u << (SCL_PIN * 2)) | (0x2u << (SDA_PIN * 2)));

    /* 5. PUPDR: internal pull-ups (01). Most OLED modules add their own 4.7k,
     *    but enabling the weak internal pulls makes a bare module work too. */
    GPIOB_PUPDR &= ~((0x3u << (SCL_PIN * 2)) | (0x3u << (SDA_PIN * 2)));
    GPIOB_PUPDR |=  ((0x1u << (SCL_PIN * 2)) | (0x1u << (SDA_PIN * 2)));

    /* 6. AFRH: AF4 (I2C2) on PB10 and PB11. (AFRH covers pins 8..15.) */
    GPIOB_AFRH &= ~((0xFu << ((SCL_PIN - 8) * 4)) | (0xFu << ((SDA_PIN - 8) * 4)));
    GPIOB_AFRH |=  ((0x4u << ((SCL_PIN - 8) * 4)) | (0x4u << ((SDA_PIN - 8) * 4)));

    /* 7. Reset the peripheral state machine, then configure while disabled. */
    I2C2_CR1 &= ~I2C_CR1_PE;
    I2C2_CR1 |=  I2C_CR1_SWRST;
    I2C2_CR1 &= ~I2C_CR1_SWRST;

    /* 8. Timing for standard mode (100 kHz) at 16 MHz APB1:
     *      FREQ  = 16  (APB1 in MHz)
     *      CCR   = APB1 / (2 * Fscl) = 16e6 / 200e3 = 80
     *      TRISE = FREQ + 1 = 17  (1000 ns max rise time) */
    I2C2_CR2   = 16u;
    I2C2_CCR   = 80u;     /* F/S = 0 (standard mode), DUTY = 0 */
    I2C2_TRISE = 17u;

    /* 9. Enable the peripheral. */
    I2C2_CR1 |= I2C_CR1_PE;
}

/* Wait until (SR1 & mask) is set, or time out. Returns 0 on success. */
static int wait_sr1(uint32_t mask)
{
    uint32_t t = I2C_TIMEOUT;
    while (!(I2C2_SR1 & mask)) {
        if (I2C2_SR1 & I2C_SR1_AF) {   /* NACK — bail */
            return -1;
        }
        if (--t == 0u) {
            return -1;
        }
    }
    return 0;
}

int i2c2_start(uint8_t addr7)
{
    /* Wait for the bus to be free (skip if it never frees — best effort). */
    uint32_t t = I2C_TIMEOUT;
    while ((I2C2_SR2 & I2C_SR2_BUSY) && --t) { }

    /* START. */
    I2C2_CR1 |= I2C_CR1_START;
    if (wait_sr1(I2C_SR1_SB) != 0) {
        return -1;
    }

    /* Address with R/W = 0 (write). Reading SR1 (above) + writing DR clears SB. */
    I2C2_DR = (uint32_t)(addr7 << 1);
    if (wait_sr1(I2C_SR1_ADDR) != 0) {
        I2C2_CR1 |= I2C_CR1_STOP;      /* NACK on address — release the bus */
        return -1;
    }

    /* Clear ADDR by reading SR1 then SR2. */
    (void)I2C2_SR1;
    (void)I2C2_SR2;
    return 0;
}

int i2c2_write_byte(uint8_t b)
{
    if (wait_sr1(I2C_SR1_TXE) != 0) {
        return -1;
    }
    I2C2_DR = (uint32_t)b;
    return 0;
}

void i2c2_stop(void)
{
    /* Make sure the last byte has fully shifted out before STOP. */
    uint32_t t = I2C_TIMEOUT;
    while (!(I2C2_SR1 & I2C_SR1_BTF) && --t) {
        if (I2C2_SR1 & I2C_SR1_AF) {
            break;
        }
    }
    I2C2_CR1 |= I2C_CR1_STOP;
}

int i2c2_write(uint8_t addr7, const uint8_t *data, uint32_t len)
{
    if (i2c2_start(addr7) != 0) {
        return -1;
    }
    for (uint32_t i = 0; i < len; i++) {
        if (i2c2_write_byte(data[i]) != 0) {
            I2C2_CR1 |= I2C_CR1_STOP;
            return -1;
        }
    }
    i2c2_stop();
    return 0;
}
