# STM32F401 Bare-Metal Register Cookbook (ARM Cortex-M4)

A collection of bare-metal C drivers and register-level implementations for the **STM32F401CCU6 (Black Pill)** using strictly **CMSIS** hardware definitions—zero HAL or Arduino abstractions.

## Hardware & Toolchain
* **MCU:** STM32F401CCU6 (ARM Cortex-M4 @ 84 MHz)
* **Debugger:** ST-Link V2 (SWD Protocol)
* **Framework:** CMSIS (Direct Register Access)
* **Build System:** PlatformIO / OpenOCD on macOS

## Repository Directory Structure

This repository is structured sequentially to document the transition from basic digital output to full peripheral integration:

* **[`01_GPIO_Registers_Blink/`](./01_GPIO_Registers_Blink)**
  * *Focus:* Toolchain validation and basic digital output. 
  * *Implementation:* Manually enables the high-speed bus clock (`RCC`) and toggles memory-mapped bits in the Port C mode (`MODER`) and data (`ODR`) registers to flash the onboard LED.
* **[`02_EXTI_NVIC_Interrupts/`](./02_EXTI_NVIC_Interrupts)**
  * *Focus:* Asynchronous hardware events.
  * *Implementation:* Bypasses polling loops by configuring External Interrupt (`EXTI`) lines and Nested Vector Interrupt Controller (`NVIC`) priorities to trap hardware state changes from an external push-button.
* **[`03_I2C_Registers_INA226/`](./03_I2C_Registers_INA226)**
  * *Focus:* Two-wire synchronous serial communication.
  * *Implementation:* Manual setup of open-drain Alternate Function pins and baud rate control registers to query identification bytes from an INA226 power monitor sensor.
* **[`04_SPI_Registers_Master/`](./04_SPI_Registers_Master)**
  * *Focus:* High-speed synchronous serial communication.
  * *Implementation:* Configures Serial Peripheral Interface (`SPI`) master settings, shift registers, and baud rate prescalers to stream raw data blocks out of the MOSI pin.
* **[`05_Logic_Analyzer_Verification/`](./05_Logic_Analyzer_Verification)**
  * *Focus:* Hardware-in-the-loop debugging.
  * *Implementation:* Utilizes an external multi-channel logic analyzer connected to the Mac to visually inspect clock phases, framing, and data timing for custom I2C and SPI driver code.
* **[`06_SPI_Peripheral_Driver/`](./06_SPI_Peripheral_Driver)**
  * *Focus:* Complex peripheral initialization.
  * *Implementation:* Integrates the bare-metal SPI engine with timed GPIO control lines to execute startup command sequences and refresh frame buffers on an ILI9341 TFT display.hello
