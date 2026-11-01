#include <stm32f4xx.h>

// Global variables (so that we can watch it in the debugger)
// volatile so it won't be optimize by the compiler
volatile uint8_t sd_response = 0xFF;    // response from the card (0x01 = card alive and in idle state)
volatile uint8_t init_status = 1;       // 0 = success, 1 = failed
volatile uint32_t sd_ok_counter = 0;    // how many times main while loop run
volatile uint32_t init_attempts = 0;    // how many init attempts

// Simple software delay (~7 clock cycles per iteration based on waveform analysis with logic analyzer)
void fn_delay(volatile uint32_t count)
{
    while(count--);
}

void SPI1_Init(void)
{
    // Enable clocks for GPIOA and SPI1
    RCC->AHB1ENR |= (1 << 0);       // GPIOA clock
    RCC->APB2ENR |= (1 << 12);      // SPI1 clock

    // Set CS high before swtiching PA4 to output, otherwise it briefly drives low and the card could see a false "selected" pulse
    GPIOA->ODR |= (1 << 4);

    // Pin modes: PA4 = Output (CS), PA5/PA6/PA7 = alternate function (CSK/MISO/MOSI)
    GPIOA->MODER &= ~((3 << (4 * 2)) | (3 << (5 * 2)) | (3 << (6 * 2)) | (3 << (7 * 2)));
    GPIOA->MODER |= ((1 << (4 * 2)) | (2 << (5 * 2)) | (2 << (6 * 2)) | (2 << (7 * 2)));

    // High speed for SPI pins
    GPIOA->OSPEEDR |= ((3 << (5 * 2)) | (3 << (6 * 2)) | (3 << (7 * 2)));

    // Pull-up on MISO (PA6): the card lets go of the line when it is not answering, so without a pull-up pin would float and read random values
    GPIOA->PUPDR &= ~(3 << (6 * 2));
    GPIOA->PUPDR |= (1 << (6 * 2));

    // Route PA5, PA6, PA7 to AF5 (SPI1)
    GPIOA->AFR[0] &= ~((0x0F << (5 * 4)) | (0x0F << (6 * 4)) | (0x0F << (7 * 4)));
    GPIOA->AFR[0] |= ((5 << (5 * 4)) | (5 << (6 * 4)) | (5 << (7 * 4)));

    // Configure SPI1 while it is disabled
    SPI1->CR1 &= ~(1 << 6);

    // Baud rate: BR = 110 -> fPCLK/128 = 16MHz / 128 = 125kHz
    // SD cards must be initialized between 100kHz and 400 kHz
    SPI1->CR1 &= ~(7 << 3);
    SPI1->CR1 |= (6 << 3);

    // Master mode, plus software slave management (SSM = 1, SSI = 1)
    // So the SPI peripheral doesn't think another master is on the bus
    SPI1->CR1 |= (1 << 2) | (1 << 9) | (1 << 8);

    // Mode 0 (CPOL = 0, CPHA = 0) is already the default, which is what SD cards use

    // Enable SPI1
    SPI1->CR1 |= (1 << 6);
}

// Send one byte and return the byte received at the same time
// SPI always shifts in and out together: to READ a byte from the card we must SEND a dummy byte (0xFF)
uint8_t SPI1_Transfer(uint8_t data)
{
    while(!(SPI1->SR & (1 << 1)));              // Wait until TXE (Transmit buffer empty)
    *((volatile uint8_t*)&SPI1->DR) = data;     // Write the byte (8-bit access)

    while(!(SPI1->SR & (1 << 0)));              // Wait until RXNE (a byte has been received)
    return (uint8_t)SPI1->DR;                   // Reading DR also clears RXNE
}

void SD_Select(void) { GPIOA->ODR &= ~(1 << 4); }   // CS LOW = card selected
void SD_Deselect(void) { GPIOA->ODR |= (1 << 4); }  // CS HIGH = card ignored

// Send 6-byte command packet and return the card's R1 response byte (command, argument, cyclic redundancy check)
uint8_t SD_Send_Command(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    // Command packet: [0x40 | cmd] [arg 31:24] [arg 23:16] [arg 15:8] [arg 7:0] [CRC]
    SPI1_Transfer(cmd | 0x40);
    SPI1_Transfer((arg >> 24) & 0xFF);
    SPI1_Transfer((arg >> 16) & 0xFF);
    SPI1_Transfer((arg >> 8) & 0xFF);
    SPI1_Transfer(arg & 0xFF);
    SPI1_Transfer(crc);

    // The card answers within a few bytes. Until then it sends 0xFF (MISO stays high)
    // A valid R1 response always has bit 7 = 0, so keep clocking until we see that
    uint8_t response = 0xFF;

    for(int i = 0; i < 10; i++)
    {
        response = SPI1_Transfer(0xFF);  // Send dummy byte to clock the response out
        if (!(response & 0x80)) break;   // Valid R1 found, stop waiting
    }

    return response;
}

// Returns 0 if the card answered CMD0 with 0x01, 1 if it never did
// NOTE : this only proves the SPI link to the card works
// The card is NOT fully initialized yet (that need CMD8, ACMD41 and so on)
uint8_t SD_Init(void)
{
    SD_Deselect();

    // 1. At least 74 clock pulses with CS HIGH to wake the card up
    // 10 bytes x 8 clocks = 80 clocks
    for(int i = 0; i <10; i++)
    {
        SPI1_Transfer(0xFF);
    }

    SD_Select();

    // 2. Send CMD0 (GO_IDLE_STATE) to put the card into SPI mode
    // CRC 0x95 is the fixed, correct CRC for CMD0 with argument 0
    // The card should answer R1 = 0x01 ("idle state, no errors")
    int retries = 0;
    sd_response = 0xFF;

    while(sd_response != 0x01)
    {
        sd_response = SD_Send_Command(0, 0, 0x95);
        retries++;
        
        if(retries > 100)
        {
            SD_Deselect();
            SPI1_Transfer(0xFF);    // Extra 8 clocks after deselect so the card can finish
            return 1;               // Failed (check wiring, power, MISO pull-up, card inserted)
        }
    }

    SD_Deselect();
    SPI1_Transfer(0xFF);        // Extra 8 clocks after deselect so the card can finish
    return 0;                   // Success: card answered 0x01
}

int main(void)
{
    SPI1_Init();

    init_status = 1;
    while(init_status != 0)
    {
        init_attempts++;
        init_status = SD_Init();

        while(init_status != 0)
        {
            fn_delay(1000000);
        }
    }

    while(1)
    {
        sd_ok_counter++;
    }
}