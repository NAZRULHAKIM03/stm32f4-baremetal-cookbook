# 01: Bare-Metal GPIO Output (Blinky)

## Objective
Verify the toolchain, ST-Link SWD connection, and PlatformIO configuration by writing a pure bare-metal C program that controls the onboard blue LED (**PC13**) of the STM32F401CCU6 without any hardware abstraction layers.

## Register Theory & Architecture
To toggle an LED on modern 32-bit ARM microcontrollers, three distinct hardware steps must be executed via direct register manipulation:

1. **Clock Enable (`RCC->AHB1ENR`)**
   * *Concept:* STM32 chips keep internal peripheral clocks shut off by default to conserve power. 
   * *Action:* Bit 2 of the Advanced High-performance Bus 1 Enable Register (`AHB1ENR`) is flipped high (`1 << 2`) to provide a digital heartbeat to `GPIOC`.
2. **Pin Mode Configuration (`GPIOC->MODER`)**
   * *Concept:* Each GPIO pin is controlled by a 2-bit field (`00` = Input, `01` = General Purpose Output, `10` = Alternate Function, `11` = Analog).
   * *Action:* Pin 13 occupies bits 26 and 27. Both bits are cleared using a bitwise mask (`&= ~(3 << 26)`), then bit 26 is set (`|= (1 << 26)`) to force PC13 into output mode.
3. **Data State Manipulation (`GPIOC->ODR`)**
   * *Concept:* The Output Data Register (`ODR`) holds the logical high (`3.3V`) or low (`0V`) state for every pin on the port.
   * *Action:* Bitwise XOR (`^= (1 << 13)`) is used inside an infinite loop to toggle the LED state alongside a crude software delay loop.

## Build & Flash Command

pio run -t upload