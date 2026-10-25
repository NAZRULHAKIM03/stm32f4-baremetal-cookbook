#include <stm32f4xx.h>

// Simple software delay function
void fn_delay(volatile uint32_t count)
{
    while(count--);
}

void SPI1_Init(void)
{
    // 1. ENABLE CLOCKS FOR GPIOA AND SPI1
    // ===================================
    RCC->AHB1ENR |= (1 << 0);   // Enable GPIOA clock (Bit 0)
    RCC->APB2ENR |= (1 << 12);  // Enable SPI1 clock (Bit 12 - note SPI1 is on APB2)

    // 2. CONFIGURE PA5 (SCK) AND PA7 (MOSI)
    // =====================================
    // Clear mode bits for PA5 and PA7
    GPIOA->MODER &= ~((3 << (5 * 2)) | (3 << (7 * 2)));
    // Set PA5 and PA7 to Alternate Function Mode (10)
    GPIOA->MODER |= (2 << (5 * 2)) | (2 << (7 * 2));

    // Set pins to High Speed
    GPIOA->OSPEEDR |= (3 << (5 * 2)) | (3 << (7 * 2));

    // Route PA5 and PA7 to Alternate Function 5 (AF5 = SPI1)
    // AFR[0] controls pins 0 through pin 7. PA5 is AFR5 (bits 20-23), PA7 is AFR7 (bits 28-31)
    GPIOA->AFR[0] &= ~((0x0F << (5 * 4)) | (0x0F << (7 * 4)));
    GPIOA->AFR[0] |= (5 << (5 * 4)) | (5 << (7 * 4));

    // 3. CONFIGURE SPI1 CONTROL REGISTER 1 (CR1)
    // ==========================================
    // Disable SPI peripheral first while configuring
    SPI1->CR1 &= ~(1 << 6);     // SPE = 0

    // Set Baud Rate Prescaler (Bits [5:3]):
    // 000 = fPCLK/2, 001 = fPCLK/4, 010 = fPCLK/8, etc. Use fPCLK/16 (011) to keep it safe and observable
    // fPCLK = APB2 (SPI1 is on APB2), fPCLK = 16MHz (default) can be up to 84Mhz on stm32f401
    SPI1->CR1 &= ~(7 << 3);
    SPI1->CR1 |= (3 << 3);

    // Set Master Selection (MSTR = 1): STM32 acts as the SPI Master ("I am the Master")
    SPI1->CR1 |= (1 << 2);

    // Set Software Slave Management (SSM = 1) and Internal Slave Select (SSI = 1)
    // SSM = 1 : Ignore/Disconnect the physical NSS pin, SSI = 1 : Internal NSS is high - the bus is free, i am allowed to transmit (NSS must be high in master mode)
    // This prevents the hardware NSS (Network Slave Select) pin from forcing a mode fault error
    SPI1->CR1 |= (1 << 9) | (1 << 8);   // Bit 9 = SSM, Bit 8 = SSI

    // Set Clock Polarity (CPOL = 0) and Clock Phase (CPHA = 0) for Mode 0 baseline
    // CPOL = 0 : SCK idles low, first edge rises.
    // CPOL = 1 : SCK idle high, first edge falls.
    // CPHA = 0 : The first bit is pre-loaded before any clock edge. The first/leading edge is samples and second/trailing edge is shifts.
    // CPHA = 1 : Nothing is pre-loaded. The first/leading edge is shifts and second/trailing edge is samples.
    // Mode 0-3 (CPOL, CPHA) : 0 = 00, 1 = 01, 2 = 10, 3 = 11
    SPI1->CR1 &= ~((1 << 1) | (1 << 0));    // Bit 1 : CPOL, Bit 0 : CPHA

    // 4. ENABLE THE SPI1 PERIPHERAL
    // =============================
    SPI1->CR1 |= (1 << 6);      // Set SPE (SPI Enable) bit
}

void SPI1_Transmit(uint8_t data)
{
    // Step 1: Wait until Transmit Buffer Empty (TXE) flag is set in SR
    while(!(SPI1->SR & (1 << 1)));

    // Step 2: Write data into the Data Register (DR)
    // This instantly triggers the hardware shift register to clock the bits out of PA7 (MOSI)
    *((volatile uint8_t*)&SPI1->DR) = data;     // 8-bit STRB (Store Byte)

    // Step 3: Wait until Busy flag (BSY) clears to ensure transmission is fully complete
    while(SPI1->SR & (1 << 7));
}

int main(void)
{
    // Initialize system clocks, GPIO alternate functions and SPI1 registers
    SPI1_Init();

    while(1)
    {
        // Blast a dummy byte (0xAA = 0b10101010) out of the MOSI pin (PA7)
        SPI1_Transmit(0xAA);

        // Delay before transmitting again
        fn_delay(1000000);      // ~0.1875s
    }
}