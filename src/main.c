#include <stdint.h>

#define RCC_BASE        0x40023800
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define GPIOD_BASE      0x40020C00
#define GPIOD_MODER     (*(volatile uint32_t *)(GPIOD_BASE + 0x00))
#define GPIOD_ODR       (*(volatile uint32_t *)(GPIOD_BASE + 0x14))

void delay(volatile uint32_t count)
{
    while(count--);
}

int main(void)
{
    RCC_AHB1ENR |= (1 << 3);
    GPIOD_MODER |= (1 << 24);
    GPIOD_MODER |= (1 << 26);

    while(1)
    {
        GPIOD_ODR ^= (1 << 12);
        GPIOD_ODR ^= (1 << 13);
        delay(500000);
    }
}
