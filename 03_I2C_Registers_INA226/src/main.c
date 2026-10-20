#include <stm32f4xx.h>

// Simple software delay function
// Takes a count parameter and loops until it decrements down to zero
void fn_delay(volatile uint32_t count)
{
    // Decrement count down to 0 (takes roughly 3 clock cycles per iteration)
    // 3 clock cycle :  Load the current value of count into a register, Subtract & Compare (subs), Branch (bne - Branch if Not Equal)
    while(count--); 
}

void I2C1_Init(void)
{
    // 1. ENABLE THE CLOCK FOR GPIOB AND I2C1
    RCC->AHB1ENR |= (1 << 1);   // Enable GPIOB clock (Bit 1)
    RCC->AHB1ENR |= (1 << 21);  // Enable I2C1 clock (Bit 21)

    // 2. CONFIGURE PB8 (SCL) AND PB9 (SDA)
    // Clear mode bits for PB8 and PB9
    GPIOB->MODER &= ~((3 << (8 * 2)) | (3 << (9 * 2)));
    // Set PB8 and PB9 to Alternate Function Mode (10)
    GPIOB->MODER |= (2 << (8 *2) | (2 << (9 * 2)));

    // Set pins to Open-Drain output type (Mandatory for I2C lines)
    GPIOB->OTYPER |= (1 << 8) | (1 << 9);

    // Set high speed for I2C pins
    GPIOB->OSPEEDR |= (3 << (8 * 2)) | (3 << (9 * 2));

    // Enable Internal Pull-Up Resistors (or rely on external pulls-ups)
    GPIOB->PUPDR &= ~((3 << (8 * 2)) | (3 << (9 * 2)));
    GPIOB->PUPDR |= ((1 << (8 * 2)) | (1 << (9 * 2)));

    // Route PB8 and PB9 to Alternate Function 4 (AF4 for I2C communication, PB8 and PB9 use I2C1)
    // AFR[1] controls pin 8 through 15. PB8 is AFRH8 (bits 0-3). PB9 is AFRH9 (bits 4-7)
    GPIOB->AFR[1] &= ~((0x0F << 0) | (0x0F << 4));
    GPIOB->AFR[1] |= ((4 << 0) | (4 << 4));

    // 3. CONFIGURE I2C1 TIMING & SPEED
    // Disable I2C peripheral first while configuring registers
    I2C1->CR1 &= ~(1 << 0);

    // Set Peripheral Clock Frequency in CR2 (APB1 runs at 16MHz by default on internal clock)
    // Write 16 into bits [5:0]
    I2C1->CR2 = 16;

    // Configure Clock Control Register (CCR) for standard Mode 100kHz I2C speed.
    // Formula for Standard Mode: CCR = PCLK1 / (2 * Target_Speed) = 16,000,000 / (2 * 100,000) = 80.
    I2C1->CCR = 80;

    // Set Maximum Rise Time in TRISE register
    // Formula: (PCLK1 in MHz) + 1 (safety margin) = 16 + 1 = 17
    I2C1->TRISE = 17;
    // TRISE is used to estimate how many clock cycles we need to wait before the voltage is gonna go up to 3.3V again, guarantees that every single bit transition happens on a clean, stable signal

    // 4. ENABLE THE I2C1 PERIPHERAL
    I2C1->CR1 |= (1 << 0);  // Set PE (Peripheral Enable) bit
}

uint16_t I2C_Read_Manufacturer_ID(uint8_t dev_addr)
{
    uint16_t id = 0;

    // Step 1: Send START condition
    I2C1->CR1 |= (1 << 8);  // Set START bit (SB)
    while (!(I2C1->SR1 & (1 << 0)));    // Wait unitl SB flag is set (SR1 stand for Status Register 1)

    // Step 2: Send Slave Address with Write bit (Address << 1 | 0)
    I2C1->DR = (dev_addr << 1);         // DR stand for Data register, we shift 1 because the last bit is to tell 0 = Write / 1 = Read
    while (!(I2C1->SR1 & (1 << 1)));    // Wait for ADDR flag (Address sent successfully)

    // Clear ADDR flag by reading SR1 then SR2 registers (ADDR flag is Hardward-cleared bits, it is READ-Only. need to perform specific sequence to clear)
    volatile int tmp = I2C1->SR1;   // We use volatile because tmp is never used, so we need to avoid it from compiler optimization
    tmp = I2C1->SR2;
    (void)tmp;  // Prevent compiler warnings --> Warning: Local variable 'tmp' set but never used.

    // Step 3: Send the internal register address we want to read (e.g., 0xFE for ID register)
    I2C1->DR = 0xFE;
    while (!(I2C1->SR1 & (1 << 7))); // Wait until Data Register Empty (TxE)

    // Step 4: Repeated START condition for Reading
    I2C1->CR1 |= (1 << 8);              // Set START bit again
    while(!(I2C1->SR1 & (1 << 0)));     // Wait for SB flag

    // Step 5: Send Slave Address with Read bit (Address << 1 | 1)
    I2C1->DR = (dev_addr << 1 | 1);
    while (!(I2C1->SR1 & (1 << 1)));    // Wait for ADDR flag

    // For 2-byte reads: send NACK on final byte & queue STOP condition before clearing ADDR
    I2C1->CR1 &= ~(1 << 10);    // Clear ACK (tells hardware to NACK the final byte) -- NACK (No Acknowledge) to signal "i am done reading, please stop"

    tmp = I2C1->SR1;    // Read SR1 then SR2 to clear the ADDR flag
    tmp = I2C1->SR2;
    (void)tmp;          // Suppress unused variable warning

    I2C1->CR1 |= (1 << 9);  // Queue a STOP condition after the last byte arrives

    // Step 6: Read high byte and low byte from Data Register (DR)
    while(!(I2C1->SR1 & (1 << 6)));     // Wait for RxNE (Receiver Not Empty)
    uint8_t high_byte = I2C1->DR;       // Reading DR does : copies the High Byte into local C variable and it clears the RxNE flag back to 0

    while(!(I2C1->SR1 & (1 << 6)));
    uint8_t low_byte = I2C1->DR;

    id = (high_byte << 8) | low_byte;
    return id;
}

int main(void)
{
    // Initialize system clocks and I2C registers
    I2C1_Init();

    while(1)
    {
        // Query the INA226 sensor at address 0x40 to read its Manufacturer ID
        // A successful read will return 0x5449 ('TI' for Texas Instruments)
        uint16_t sensor_id = I2C_Read_Manufacturer_ID(0x40);
        (void)sensor_id;   // prevent compiler warning (unused variable)

        // Delay before polling again
        // Pause for roughly 375 milliseconds (0.375s) before polling the sensor again
        // 2,000,000 counts * ~3 assembly cycles / 16,000,000 (16MHz) clock speed = 0.375s
        fn_delay(2000000);
    }
}