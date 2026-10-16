# 02: Hardware Interrupts (EXTI & NVIC)

## Objective
Ditch software polling loops and configure the **EXTI** (External Interrupt) and **NVIC** (Nested Vector Interrupt Controller) registers so a physical button press on **PA0** automatically triggers an asynchronous hardware interrupt to toggle the onboard LED (**PC13**).

## Register Theory & Architecture
Interrupt handling on the ARM Cortex-M4 involves four core silicon layers:
1. **GPIO Configuration (`GPIOA->MODER` & `PUPDR`)**: Configures pin PA0 as a digital input with an internal pull-up resistor (default 3.3V high).
2. **SYSCFG Routing (`SYSCFG->EXTICR`)**: Maps Port A, Pin 0 to External Interrupt Line 0 (`EXTI0`).
3. **EXTI Masking & Trigger (`EXTI->IMR` & `FTSR`)**: Unmasks Line 0 and configures it to trigger on a falling edge (when the button pulls the voltage down to 0V).
4. **NVIC Core Enable (`NVIC_EnableIRQ`)**: Enables the global `EXTI0_IRQn` vector inside the ARM Cortex-M4 core router.
5. **Pending Register Clear (`EXTI->PR`)**: Uses the Write-1-to-Clear (`W1C`) rule to reset the interrupt flag inside the ISR so the CPU does not loop endlessly.

## Build & Flash Command

pio run -t upload