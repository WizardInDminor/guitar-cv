#include "i2c1.h"

/*
 * Register definitions (STM32F407). Inline-register style, matching src/spi2.c.
 * Addresses/bit fields per docs/reference/register-map.md.
 */
#define RCC_AHB1ENR   (*(volatile uint32_t *)0x40023830) /* bit 1  GPIOBEN */
#define RCC_APB1ENR   (*(volatile uint32_t *)0x40023840) /* bit 21 I2C1EN  */

#define GPIOB_MODER   (*(volatile uint32_t *)0x40020400)
#define GPIOB_OTYPER  (*(volatile uint32_t *)0x40020404)
#define GPIOB_OSPEEDR (*(volatile uint32_t *)0x40020408)
#define GPIOB_PUPDR   (*(volatile uint32_t *)0x4002040C)
#define GPIOB_AFRL    (*(volatile uint32_t *)0x40020420) /* pins 0..7 */

#define I2C1_CR1      (*(volatile uint32_t *)0x40005400)
#define I2C1_CR2      (*(volatile uint32_t *)0x40005404)
#define I2C1_DR       (*(volatile uint32_t *)0x40005410)
#define I2C1_SR1      (*(volatile uint32_t *)0x40005414)
#define I2C1_SR2      (*(volatile uint32_t *)0x40005418)
#define I2C1_CCR      (*(volatile uint32_t *)0x4000541C)
#define I2C1_TRISE    (*(volatile uint32_t *)0x40005420)

/* I2C1_CR1 bits. */
#define CR1_PE        (1u << 0)   /* peripheral enable     */
#define CR1_START     (1u << 8)   /* generate START        */
#define CR1_STOP      (1u << 9)   /* generate STOP         */
#define CR1_SWRST     (1u << 15)  /* software reset        */

/* I2C1_SR1 status bits. */
#define SR1_SB        (1u << 0)   /* start condition sent  */
#define SR1_ADDR      (1u << 1)   /* address phase done    */
#define SR1_BTF       (1u << 2)   /* byte transfer finish  */
#define SR1_TxE       (1u << 7)   /* TX data register empty */

/* I2C1_SR2 status bits. */
#define SR2_BUSY      (1u << 1)   /* bus busy              */

/* Pin numbers on GPIOB. */
#define SCL_PIN  6   /* PB6 — I2C1_SCL (AF4) */
#define SDA_PIN  7   /* PB7 — I2C1_SDA (AF4) */

i2c_status_t i2c1_init(uint32_t pclk1_hz, uint32_t scl_hz)
{
    /* 0. Timing from the actual APB1 clock; validate the field limits. */
    uint32_t freq  = i2c_freq_field(pclk1_hz);
    uint32_t ccr   = i2c_sm_ccr(pclk1_hz, scl_hz);
    uint32_t trise = i2c_sm_trise(pclk1_hz);

    if (scl_hz == 0u || scl_hz > 100000u ||   /* standard mode only      */
        freq < 2u || freq > 50u ||            /* CR2.FREQ field limits   */
        ccr < 4u || ccr > 0xFFFu ||           /* SM minimum / 12-bit CCR */
        trise > 63u) {                        /* 6-bit TRISE field       */
        return I2C_ERR_INVALID_CONFIG;
    }

    /* 1. Enable GPIOB and I2C1 clocks. */
    RCC_AHB1ENR |= (1u << 1);   /* GPIOBEN */
    RCC_APB1ENR |= (1u << 21);  /* I2C1EN  */

    /* 2. MODER: PB6/PB7 alternate function (10). */
    GPIOB_MODER &= ~((0x3u << (SCL_PIN * 2)) | (0x3u << (SDA_PIN * 2)));
    GPIOB_MODER |=  ((0x2u << (SCL_PIN * 2)) | (0x2u << (SDA_PIN * 2)));

    /* 3. OTYPER: open-drain (1) — required for I2C wired-AND bus. */
    GPIOB_OTYPER |= (1u << SCL_PIN) | (1u << SDA_PIN);

    /* 4. OSPEEDR: medium speed (01). PUPDR: pull-up (01). */
    GPIOB_OSPEEDR &= ~((0x3u << (SCL_PIN * 2)) | (0x3u << (SDA_PIN * 2)));
    GPIOB_OSPEEDR |=  ((0x1u << (SCL_PIN * 2)) | (0x1u << (SDA_PIN * 2)));
    GPIOB_PUPDR   &= ~((0x3u << (SCL_PIN * 2)) | (0x3u << (SDA_PIN * 2)));
    GPIOB_PUPDR   |=  ((0x1u << (SCL_PIN * 2)) | (0x1u << (SDA_PIN * 2)));

    /* 5. AFRL: AF4 (I2C1) on PB6 and PB7. */
    GPIOB_AFRL &= ~((0xFu << (SCL_PIN * 4)) | (0xFu << (SDA_PIN * 4)));
    GPIOB_AFRL |=  ((0x4u << (SCL_PIN * 4)) | (0x4u << (SDA_PIN * 4)));

    /* 6. Software reset to clear any stuck state, then reconfigure.
     *    CCR and TRISE must be written while PE=0. */
    I2C1_CR1 = CR1_SWRST;
    I2C1_CR1 = 0;

    /* 7. Standard-mode timing computed from PCLK1 (step 0):
     *    16 MHz -> FREQ=16, CCR=80,  TRISE=17
     *    42 MHz -> FREQ=42, CCR=210, TRISE=43 */
    I2C1_CR2   = freq;
    I2C1_CCR   = ccr;
    I2C1_TRISE = trise;

    /* 8. Enable I2C1 (PE last). */
    I2C1_CR1 = CR1_PE;

    return I2C_OK;
}

void i2c1_write(uint8_t addr7, const uint8_t *buf, uint16_t len)
{
    /* Wait until bus is free. */
    while (I2C1_SR2 & SR2_BUSY) { }

    /* Generate START. */
    I2C1_CR1 |= CR1_START;
    while (!(I2C1_SR1 & SR1_SB)) { }        /* wait SB=1 */

    /* Send 7-bit address + write bit; clears SB. */
    I2C1_DR = (uint8_t)(addr7 << 1);
    while (!(I2C1_SR1 & SR1_ADDR)) { }      /* wait ADDR=1 */
    (void)I2C1_SR2;                          /* clear ADDR: read SR1 then SR2 */

    /* Send each byte. */
    for (uint16_t i = 0; i < len; i++) {
        while (!(I2C1_SR1 & SR1_TxE)) { }   /* wait TX register empty */
        I2C1_DR = buf[i];
    }

    /* Wait for shift register to drain, then generate STOP. */
    while (!(I2C1_SR1 & SR1_BTF)) { }
    I2C1_CR1 |= CR1_STOP;
}
