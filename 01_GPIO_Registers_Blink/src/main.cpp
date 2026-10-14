#include "stm32f4xx.h"      // Master dictionary of all hardware register addresses

void delay(volatile uint32_t count)
{
    while(count--)
    {
        // Volatile prevents the compiler from optimizing away this empty loop
    }
}

int main(void)
{
    // 1. ENABLE THE CLOCK FOR PORT C
    // STM32 shuts off power to all GPIO ports by default to save battery.
    // The LED is on Pin PC13. We must turn on the clock for GPIOC.
    // AHB1ENR is the "Advanced High-performance Bus 1 Enable Register".
    // Bit 2 controls GPIOC. Use Bitwise OR (|=) to flip bit 2 to a '1'.
    RCC->AHB1ENR |= (1<<2);

    // 2. CONFIGURE PC13 AS AN OUTPUT
    // The MODER register controls pin modes (Input, Output, Analog, Alternate).
    // Each pin gets 2 bits. PC13 is controlled by bits 26 and 27. t
    // First, we clear both bits using bitwise AND (&) with a NOT (~) mask.
    GPIOC->MODER &= ~(3 << 26);

    // Next, we set bit 26 to '1' (Output Mode is binary 01).
    GPIOC->MODER |= (1 << 26);

    // 3. THE INFINITE LOOP
    while(1)
    {
        // ODR is the "Output Data Register".
        // We use bitwise XOR (^=) on bit 13 to flip the LED state.
        GPIOC->ODR ^= (1 << 13);

        // Crude delay (approx 1 seconq based on the default 16MHz internal clock)
        delay(1000000);
    }
}