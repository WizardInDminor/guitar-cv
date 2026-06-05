#include "spi2.h"

/*
 * Register definitions (STM32F407). Inline-register style, matching src/main.c.
 * Addresses/bit fields per docs/reference/register-map.md.
 */
#define RCC_AHB1ENR   (*(volatile uint32_t *)0x40023830) /* bit 1  GPIOBEN */
#define RCC_APB1ENR   (*(volatile uint32_t *)0x40023840) /* bit 14 SPI2EN  */

#define GPIOB_MODER   (*(volatile uint32_t *)0x40020400)
#define GPIOB_OTYPER  (*(volatile uint32_t *)0x40020404)
#define GPIOB_OSPEEDR (*(volatile uint32_t *)0x40020408)
#define GPIOB_PUPDR   (*(volatile uint32_t *)0x4002040C)
#define GPIOB_ODR     (*(volatile uint32_t *)0x40020414)
#define GPIOB_AFRH    (*(volatile uint32_t *)0x40020424)

#define SPI2_CR1      (*(volatile uint32_t *)0x40003800)
#define SPI2_SR       (*(volatile uint32_t *)0x40003808)
#define SPI2_DR16     (*(volatile uint16_t *)0x4000380C) /* halfword: DFF=1 */

/* Pin numbers on GPIOB. */
#define CS_PIN   12   /* PB12 — software chip select         */
#define SCK_PIN  13   /* PB13 — SPI2_SCK  (AF5)              */
#define MOSI_PIN 15   /* PB15 — SPI2_MOSI (AF5)              */

/* SPI2_SR status bits. */
#define SPI_SR_TXE  (1u << 1)  /* transmit buffer empty */
#define SPI_SR_BSY  (1u << 7)  /* bus busy              */

void spi2_init(void)
{
    /* 1. Enable GPIOB and SPI2 clocks. */
    RCC_AHB1ENR |= (1u << 1);
    RCC_APB1ENR |= (1u << 14);

    /* 2. MODER: PB12 output (01), PB13/PB15 alternate function (10). */
    GPIOB_MODER &= ~((0x3u << (CS_PIN * 2)) |
                     (0x3u << (SCK_PIN * 2)) |
                     (0x3u << (MOSI_PIN * 2)));
    GPIOB_MODER |=  ((0x1u << (CS_PIN * 2)) |
                     (0x2u << (SCK_PIN * 2)) |
                     (0x2u << (MOSI_PIN * 2)));

    /* 3. AFRH: AF5 (SPI2) on PB13 and PB15. (AFRH covers pins 8..15.) */
    GPIOB_AFRH &= ~((0xFu << ((SCK_PIN  - 8) * 4)) |
                    (0xFu << ((MOSI_PIN - 8) * 4)));
    GPIOB_AFRH |=  ((0x5u << ((SCK_PIN  - 8) * 4)) |
                    (0x5u << ((MOSI_PIN - 8) * 4)));

    /* 4. High speed (10), push-pull (0), no pull (00) on PB12/13/15. */
    GPIOB_OSPEEDR &= ~((0x3u << (CS_PIN * 2)) |
                       (0x3u << (SCK_PIN * 2)) |
                       (0x3u << (MOSI_PIN * 2)));
    GPIOB_OSPEEDR |=  ((0x2u << (CS_PIN * 2)) |
                       (0x2u << (SCK_PIN * 2)) |
                       (0x2u << (MOSI_PIN * 2)));
    GPIOB_OTYPER &= ~((1u << CS_PIN) | (1u << SCK_PIN) | (1u << MOSI_PIN));
    GPIOB_PUPDR  &= ~((0x3u << (CS_PIN * 2)) |
                      (0x3u << (SCK_PIN * 2)) |
                      (0x3u << (MOSI_PIN * 2)));

    /* 5. Idle CS high before enabling SPI. */
    GPIOB_ODR |= (1u << CS_PIN);

    /* 6. CR1: DFF=1, SSM=1, SSI=1, BR=/8, MSTR=1; CPOL=0, CPHA=0, MSB first. */
    SPI2_CR1 = (1u << 11) |  /* DFF  16-bit frame        */
               (1u << 9)  |  /* SSM  software CS          */
               (1u << 8)  |  /* SSI  internal NSS high    */
               (2u << 3)  |  /* BR   fPCLK/8 (~2 MHz)     */
               (1u << 2);    /* MSTR master               */

    /* 7. Enable SPI2 (SPE last). */
    SPI2_CR1 |= (1u << 6);
}

void spi2_write16(uint16_t data)
{
    GPIOB_ODR &= ~(1u << CS_PIN);              /* assert CS              */

    while (!(SPI2_SR & SPI_SR_TXE)) { }        /* wait TX buffer empty   */
    SPI2_DR16 = data;                          /* halfword write (DFF=1) */
    while (SPI2_SR & SPI_SR_BSY) { }           /* wait transfer complete */

    GPIOB_ODR |= (1u << CS_PIN);               /* deassert CS            */
}
