# 06: SPI MicroSD Card Initialization

## Objective
Implement a robust, production-grade bare-metal SPI driver and state machine to wake up a physical MicroSD card, switch it from default SDIO mode into SPI mode, and execute the low-level `CMD0` hardware reset handshake—bypassing high-level file system libraries entirely.

## Register Theory & Architecture
Communicating with storage media over SPI requires precise power-up sequencing, bus protection and signal timing:
1. **Startup Glitch Prevention (`GPIOA->ODR`)**: Explicitly sets the Chip Select (`CS` / `PA4`) high *before* configuring the pin mode to prevent accidental low-level pulses from tricking the card's state machine.
2. **Optimized Initialization Baud Rate (`SPI1->CR1`)**: Programs the prescaler to `fPCLK/128` (125 kHz based on a 16MHz APB2 bus) to sit safely inside the mandatory SD specification window of **100 kHz to 400 kHz** for initialization.
3. **MISO Internal Pull-Up (`GPIOA->PUPDR`)**: Activates the internal pull-up resistor on **PA6 (MISO)**. Because SD cards release the data line when idle, this prevents floating pin values and random bit errors.
4. **Clock Train & Post-Deselect Tail (`SD_Init`)**: Generates the required 74+ clock pulses with `CS` high to wake the card, followed by an extra 8 clock cycles (`SPI1_Transfer(0xFF)`) after each deselect action to give the card time to finalize its internal cycle.
5. **Software Reset & Verification (`CMD0`)**: Pulls `CS` low and transmits the 6-byte reset packet (`0x40, 0x00, 0x00, 0x00, 0x00, 0x95`), confirming the SPI link is active and the card is successfully placed into Idle State (`R1 response = 0x01`).

## Hardware Wiring
| MicroSD Module Pin | STM32F401CCU6 Pin | Description |
| :--- | :--- | :--- |
| **CS** | PA4 | Chip Select (Active Low) |
| **SCK** | PA5 | SPI1 Serial Clock |
| **MISO** | PA6 | SPI1 Master In / Slave Out (Card Response with internal pull-up) |
| **MOSI** | PA7 | SPI1 Master Out / Slave In (Command Stream) |
| **VCC** | 5V | Power Supply |
| **GND** | GND | Common Ground |

## Build & Flash Command

pio run -t upload