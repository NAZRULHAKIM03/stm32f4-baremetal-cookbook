# STM32F4 Bare-Metal Cookbook

Bare-metal C drivers for the **STM32F401CCU6 (Black Pill, ARM Cortex-M4)**, written directly against the registers using only **CMSIS** headers. No HAL, no Arduino framework.

I built this to understand what happens underneath the libraries: which bits to set, in what order, and how to prove with a logic analyzer that the signals on the wire are correct. Each folder is one small step, from blinking an LED to talking to an SD card.

## Hardware and Toolchain

| Item | Details |
|---|---|
| MCU | STM32F401CCU6 (Black Pill) |
| Clock | Default 16 MHz internal oscillator (HSI), so APB1 and APB2 both run at 16 MHz. The timing values in the I2C and SPI code (CCR, TRISE, baud prescaler) are calculated for this |
| Debugger | ST-Link V2 |
| Framework | CMSIS (direct register access) |
| Build system | PlatformIO in VS Code (macOS) |
| Test equipment | 8-channel logic analyzer |
| Sensor and module | INA226 power monitor (I2C), microSD card module (SPI) |

## Repository Structure

```
stm32f4-baremetal-cookbook/
├── 01_GPIO_Registers_Blink/
├── 02_EXTI_NVIC_Interrupts/
├── 03_I2C_Registers_INA226/
├── 04_SPI_Registers_Master/
├── 05_Logic_Analyzer_Verification/
├── 06_SPI_SD_Card/
├── .gitignore
└── README.md
```

Folders 01 to 04 and 06 each contain `src/main.c`, a `platformio.ini` and their own `README.md`. Folder 05 contains the analyzer captures and my analysis notes.

## What Each Folder Covers

| Folder | Topic | What it does |
|---|---|---|
| [`01_GPIO_Registers_Blink`](./01_GPIO_Registers_Blink) | GPIO and clocks | Enables the GPIOC clock in `RCC` and sets the `MODER` and `ODR` bits of PC13 to blink the onboard LED. Confirms the toolchain and ST-Link work |
| [`02_EXTI_NVIC_Interrupts`](./02_EXTI_NVIC_Interrupts) | Interrupts | Configures an `EXTI` line and the `NVIC` so a rotary encoder push button triggers an interrupt, replacing polling and software delays |
| [`03_I2C_Registers_INA226`](./03_I2C_Registers_INA226) | I2C master | Sets up I2C1 on PB8 (SCL) and PB9 (SDA) at 100 kHz and reads the INA226 manufacturer ID register. A correct read returns `0x5449` |
| [`04_SPI_Registers_Master`](./04_SPI_Registers_Master) | SPI master | Configures SPI1 as a master and sends a test byte (`0xAA`) on MOSI |
| [`05_Logic_Analyzer_Verification`](./05_Logic_Analyzer_Verification) | Verification | Logic analyzer captures and handwritten waveform notes for the I2C (03) and SPI (04) drivers |
| [`06_SPI_SD_Card`](./06_SPI_SD_Card) | SPI peripheral | Uses the SPI driver to wake a microSD card and switch it into SPI mode (`CMD0`) |

## Highlights

### I2C: reading the INA226 manufacturer ID (03)

- Open-drain alternate-function pins (AF4), with `CR2`, `CCR` and `TRISE` computed for 100 kHz standard mode.
- Full master-receiver sequence: START, address with write bit, register pointer, repeated START, address with read bit, then a 2-byte read.
- A 2-byte read needs a special sequence (`POS` and `ACK` set before clearing `ADDR`, `ACK` cleared, wait for `BTF`, then `STOP` before reading `DR`). The simple 1-byte recipe makes the second byte never arrive.
- Result: the driver reads back `0x5449` ("TI") from the INA226.

### SPI and the logic analyzer (04 and 05)

- SPI1 master, mode 0, software slave management, with SCK on PA5 and MOSI on PA7 (AF5).
- Captured with the analyzer to check clock count, clock phase and the bit pattern of the data. The `.sr` session files can be opened in PulseView.
- My handwritten notes on the waveforms are in `05_Logic_Analyzer_Verification/captures/Note_Waveform_Analysis.pdf`.

### SD card over SPI (06)

- Pins: PA4 = CS, PA5 = SCK, PA6 = MISO, PA7 = MOSI. MISO has the internal pull-up enabled, because the card lets go of the line when it is not answering.
- SPI1 runs at 125 kHz (fPCLK/128) because SD cards must be initialized between 100 and 400 kHz.
- Sequence: at least 74 clocks with CS high, then `CMD0` (`0x40 00 00 00 00 95`). A card in idle state answers `R1 = 0x01`.
- The code exposes `sd_response` and `init_status` as global variables so they can be watched in the debugger.
- **Scope:** this proves the SPI link to the card works in both directions. The card is not fully initialized yet. That needs `CMD8`, an `ACMD41` loop and `CMD58`, which are the next step.

## Verified Results

| Test | Result |
|---|---|
| INA226 manufacturer ID over I2C | Reads `0x5449` |
| I2C and SPI signals on the logic analyzer | Captured and checked (see folder 05) |
| SD card response to `CMD0` | `R1 = 0x01` |

## Build and Flash

1. Install VS Code and the PlatformIO extension.
2. Open one project folder (for example `03_I2C_Registers_INA226`) in PlatformIO. Each folder is its own project with its own `platformio.ini`.
3. Connect the Black Pill through an ST-Link V2.
4. Build and upload from PlatformIO, or run `pio run -t upload` in the project folder.