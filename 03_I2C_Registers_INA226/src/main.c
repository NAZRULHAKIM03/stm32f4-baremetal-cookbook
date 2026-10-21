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
    RCC->APB1ENR |= (1 << 21);  // Enable I2C1 clock (Bit 21)

    // 2. CONFIGURE PB8 (SCL) AND PB9 (SDA)
    // Clear mode bits for PB8 and PB9
    GPIOB->MODER &= ~((3 << (8 * 2)) | (3 << (9 * 2)));
    // Set PB8 and PB9 to Alternate Function Mode (10)
    GPIOB->MODER |= (2 << (8 *2)) | (2 << (9 * 2));

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

    // 4. ENABLE THE I2C1 PERIPHERAL, THEN ACK
    I2C1->CR1 |= (1 << 0);  // Set PE first (ACK is forced to 0 while PE = 0)
    I2C1->CR1 |= (1 << 10); // ACK enable (only sticks once PE = 1)
}

uint16_t I2C_Read_Manufacturer_ID(uint8_t dev_addr)
{
    uint16_t id = 0;
    volatile int tmp;   // Only used to read SR1/SR2 (volatile so compiler doesn't remove the reads)

    // Step 1: Send START condition
    I2C1->CR1 |= (1 << 8);              // Set START bit
    while(!(I2C1->SR1 & (1 << 0)));     // Wait until SB flag is set (Start condition generated)

    // Step 2: Send Slave Address with Write bit (Address << 1 | 0)
    I2C1->DR = (dev_addr << 1);         // Last bit: 0 = Write / 1 = Read
    while(!(I2C1->SR1 & (1 << 1)));     // Wait for ADDR flag (slave ACKed its address)

    // Clear ADDR flag: read SR1 then SR2 (hardware only clears it after this exact sequence)
    tmp = I2C1->SR1;
    tmp = I2C1->SR2;
    (void)tmp;                          // Avoid compiler warning (unused variable)

    // Step 3: Send the internal register address we want to read (0xFE = Manufacturer ID)
    I2C1->DR = 0xFE;
    // Wait for BTF (Byte Transfer Finished) so the byte is fully shifted out onto the bus before we issue the repeated START
    while(!(I2C1->SR1 & (1 << 2)));     // BTF = Byte Transfer Finished

    // Step 4: Repeated START condition (switch from writing to reading)
    I2C1->CR1 |= (1 << 8);
    while(!(I2C1->SR1 & (1 << 0)));     // Wait for SB flag

    // Step 5: Send Slave Address with Read bit (Address << 1 | 1)
    I2C1->DR = (dev_addr << 1) | 1;
    while(!(I2C1->SR1 & (1 << 1)));     // Wait for ADDR flag

    // Special sequence for receiving EXACTLY 2 bytes (the 1-byte recipe doesn't work here):
    // POS = 1 makes the ACK bit control the NEXT byte (the one that will arrive in the shift register next),
    // instead of the byte currently being received. without it, we had NACK the first byte
    // and the slave would stop sending, which cause the low_byte never arrived
    I2C1->CR1 |= (1 << 11);     // POS = 1
    I2C1->CR1 |= (1 << 10);     // ACK = 1 (make sure ACK is on before the transfer starts)

    // Clear ADDR flag (Read SR1 then SR2). This must happen AFTER setting POS/ACK above because clearing ADDR
    // is what lets the first byte start coming in
    tmp = I2C1->SR1;
    tmp = I2C1->SR2;
    (void)tmp;

    // Now clear ACK so the 2nd (last) byte gets a NACK = "I am done reading, stop"
    I2C1->CR1 &= ~(1 << 10);    // ACK = 0

    // Step 6: Wait for BTF. At this moment, byte 1 is sitting in DR and byte 2 is already
    // in the shift register, so both bytes have arrived and the clock is held (strecthed) because BTF = 1
    while(!(I2C1->SR1 & (1 << 2)));

    I2C1->CR1 |= (1 << 9);      // Queue STOP now, before reading DR (reading DR releases the clock)

    uint8_t high_byte = I2C1->DR;   // Read byte 1 (this also move byte 2 from shift register into DR)
    uint8_t low_byte = I2C1->DR;    // Read byte 2

    // Put things back to normal so the next call starts clean
    I2C1->CR1 &= ~(1 << 11);        // POS = 0
    I2C1->CR1 |= (1 << 10);         // ACK = 1

    id = ((uint16_t)high_byte << 8) | low_byte;
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
        volatile uint16_t sensor_id = I2C_Read_Manufacturer_ID(0x40);
        (void)sensor_id;

        // Delay before polling again
        // Pause for roughly 375 milliseconds (0.375s) before polling the sensor again
        // 2,000,000 counts * ~3 assembly cycles / 16,000,000 (16MHz) clock speed = 0.375s
        fn_delay(2000000);
    }
}