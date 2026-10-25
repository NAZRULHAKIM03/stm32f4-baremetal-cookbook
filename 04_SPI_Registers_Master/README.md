# 04: Bare-Metal SPI Master

## Objective
Configure the **SPI (Serial Peripheral Interface)** peripheral manually using direct memory-mapped registers to transmit raw bytes out of the MOSI pin—bypassing high-level libraries entirely.

## Register Theory & Architecture
High-speed synchronous serial communication requires configuring four hardware layers:
1. **Alternate Function Configuration (`GPIOA->MODER` & `AFR`)**: Pins **PA5 (SCK)** and **PA7 (MOSI)** are set to Alternate Function Mode (`10`) and mapped to **AF5** (SPI1 multiplexer).
2. **Bus Clock Control (`RCC`)**: Enables high-speed peripheral clocks for `GPIOA` (`AHB1ENR`) and `SPI1` (`APB2ENR` on the high-speed APB2 bus).
3. **SPI Control Register (`SPI1->CR1`)**: Configures Master mode (`MSTR = 1`), Software Slave Management (`SSM = 1` / `SSI = 1`), and baud rate prescalers (`BR`).
4. **Data Transmission Engine (`SPI1->SR` & `DR`)**: Polls the Transmit Buffer Empty (`TXE`) flag, loads bytes into the Data Register (`DR`) to trigger hardware shifting, and waits for the Busy (`BSY`) flag to clear.

## Hardware Wiring Map
| Pin Name | STM32F401CCU6 Pin | Description |
| :--- | :--- | :--- |
| **SCK** | PA5 | SPI1 Serial Clock (Master Output) |
| **MOSI** | PA7 | SPI1 Master Out / Slave In Data Line |

## Build & Flash Command

pio run -t upload