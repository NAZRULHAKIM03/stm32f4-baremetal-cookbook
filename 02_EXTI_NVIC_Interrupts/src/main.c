#include "stm32f4xx.h"      // Master dictionary of all hardware register addresses

int main()
{
    // 1. ENABLE THE CLOCK FOR GPIOA, GPIOC and SYSCFG
    // Bit 0 controls GPIOA (for Button on PA0)
    RCC->AHB1ENR |= (1 << 0);
    // Bit 2 controls GPIOC (for the onboard LED on PC13)
    RCC->AHB1ENR |= (1 << 2);
    // SYSCFG manages external interrupt line routing
    // must enable its clock via APB2 (Advanced Peripheral Bus 2) bus register before configuring interrupts
    RCC->APB2ENR |= (1 << 14);

    // 2. CONFIGURE PC13 AS OUTPUT (LED)
    // Each pin gets 2 bits. PC13 is controlled by bits 26 and 27
    // Set to 01 (Output Mode)
    GPIOC->MODER &= ~(3 << 26);     // clear bits 26, 27
    GPIOC->MODER |= (1 << 26);      // set to 01

    // 3. CONFIGURE PA0 AS INPUT WITH PULL-UP
    // Set PA0 to Input Mode (00)
    GPIOA->MODER &= ~(3 << 0);
    // Enable Internal Pull-Up Resistor on PA0 (PUPDR register: '01' = Pull-up)
    // PUPDR (Pull Up Pull Down Register)
    // This keeps PA0 at 3.3V high until the button pulls it to ground (0V)
    GPIOA->PUPDR &= ~(3 << 0);
    GPIOA->PUPDR |= (1 << 0);

    // 4. CONFIGURE SYSCFG (ROUTE PA0 TO EXTI0)
    // SYSCFG_EXTICR1(EXTICR[0]) controls EXTI lines 0 to 3, [1] controls lines 4 to 7 and so on until [3]. line = pin number
    // Bits [3:0] control EXTI0. Writing '0000' (or 0) maps PA0 to EXTI0, '0001' maps PB0 to EXTI0 and so on. [3:0] = EXTI0, [7:4] = EXTI1, [11:8] = EXTI2, [15:12] = EXTI3
    SYSCFG->EXTICR[0] &= ~(0x0F << 0);      // clear bits for EXTI0

    // 5. CONFIGURE EXTI LINE 0 (INTERRUPT MASK & TRIGGER)
    // Unmask EXTI Line 0 so it doesn't ignore incoming signals
    EXTI->IMR |= (1 << 0);

    // Set Falling Trigger Selection Register (FTSR) for Line 0
    // A falling edge occurs when the button is pressed (3.3V -> GND)
    EXTI->FTSR |= (1 << 0);

    // 6. CONFIGURE NVIC (ARM CORE INTERRUPT ROUTER)
    // EXTI Line 0 has a dedicated interrupt vector named EXTI0_IRQn in CMSIS
    // We tell the ARM Core to enable this specific interrupt line
    NVIC_EnableIRQ(EXTI0_IRQn);

    // 7. THE INFINITE LOOP (DO NOTHING FOR NOW)
    while(1)
    {
        // The CPU is completely free to do background tasks or stay idle
        // The LED toggle is handled entirely by hardware interrupts
    }
}

// 8. THE INTERRUPT SERVICE ROUTINE (ISR)
// This function name is a fixed vector name defined in the startup file
// When EXTI0 fires, the CPU automatically pauses 'main' and jumps straight here
void EXTI0_IRQHandler(void)
{
    // Check if the interrupt flag for line 0 is actually set
    if (EXTI->PR & (1 << 0))
    {
        // Toggle the onboard blue LED on PC13
        GPIOC->ODR ^= (1 << 13);

        // CLEAR THE PENDING REGISTER (PR) FLAG!!!
        // This is a mandatory hardware rule : if we didn't clear the flag by writing a '1'
        // to it, the CPU will think the interrupt is still happening and loop here forever
        EXTI->PR |= (1 << 0);
    }
}