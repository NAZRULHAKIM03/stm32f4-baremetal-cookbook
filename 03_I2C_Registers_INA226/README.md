# 03: Bare-Mail I2C Master (INA226 Power Monitor)

## Objective
Configure the **I2C (Inter-Integrated Circuit)** peripheral manually using direct memory-mapped registers to query identification data from an INA226 power monitor sensor—bypassing high-level libraries entirely.

## Register Theory & Architecture
Communicating over a two-wire synchronous serial bus requires configuring five hardware layers:
1. **Alternate Function Configuration (`GPIOB->MODER` & `AFR`)**: Pins **PB8 (SCL)** and **PB9 (SDA)** are set to Alternate Function Mode (`10`) and mapped to **AF4** (I2C1 multiplexer).
2. **Open-Drain Output (`GPIOB->OTYPER`)**: Configures pins to open-drain mode, allowing devices to safely pull the bus voltage low without short-circuiting.
3. **Bus Clock Control (`RCC`)**: Enables the high-speed peripheral clocks for `GPIOB` (`AHB1ENR`) and `I2C1` (`APB1ENR`).
4. **Timing & Speed Configuration (`I2C1->CR2`, `CCR`, `TRISE`)**: Calculates and sets the clock control register for Standard Mode (100 kHz) operations based on the APB1 clock frequency.
5. **Peripheral Activation (`I2C1->CR1`)**: Toggles the Peripheral Enable (`PE`) bit and manually handles master communication sequences (`START`, address shifts, status register polling `SR1`/`SR2`, and `STOP`).

## Hardware Wiring Map
| INA226 Sensor Pin | STM32F401CCU6 Pin | Description |
| :--- | :--- | :--- |
| **SCL** | PB8 | I2C1 Serial Clock Line |
| **SDA** | PB9 | I2C1 Serial Data Line |
| **VCC** | 3.3V | Power Supply |
| **GND** | GND | Common Ground |

## Build & Flash Command

pio run -t upload