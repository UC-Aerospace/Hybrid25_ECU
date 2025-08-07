/**
 * @file hal.c
 *
 * @brief Example of a hardware abstraction layer
 * @warning This software utilizes TI Drivers
 *
 * @copyright Copyright (C) 2022 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "hal.h"
#include "ads124.h"
#include "stm32g0xx_hal.h"
#include "config.h"

// SPI configuration - using STM32 SPI1 peripheral
extern SPI_HandleTypeDef hspi1;

// Internal helper function to get system clock frequency
static uint32_t getSysClockHz(void)
{
    return HAL_RCC_GetSysClockFreq();
}



//****************************************************************************
//
// Internal variables
//
//****************************************************************************


//****************************************************************************
//
// Internal function prototypes
//
//****************************************************************************
void InitGPIO(void);
void InitSPI(void);
void GPIO_DRDY_IRQHandler(uint_least8_t index);

//****************************************************************************
//
// External Functions (prototypes declared in hal.h)
//
//****************************************************************************



//****************************************************************************
//
// Timing functions
//
//****************************************************************************

/************************************************************************************//**
 *
 * @brief delay_ms()
 *          Provides a timing delay with 'ms' resolution.
 *
 * @param[in] delay_time_ms Is the number of milliseconds to delay.
 *
 * @return none
 */
void delay_ms(const uint32_t delay_time_ms)
{
    HAL_Delay(delay_time_ms);
}

/************************************************************************************//**
 *
 * @brief delay_us()
 *          Provides a timing delay with 'us' resolution.
 *
 * @param[in] delay_time_us Is the number of microseconds to delay.
 *
 * @return none
 */
void delay_us(const uint32_t delay_time_us)
{
    // For STM32G0, we'll use a simple loop-based delay for microseconds
    // This is a rough approximation and may need calibration for precise timing
    uint32_t cycles = delay_time_us * (getSysClockHz() / 1000000U);
    
    // Simple loop delay (approximately 3-4 cycles per iteration)
    for(uint32_t i = 0; i < cycles/4; i++) {
        __NOP(); // No operation - prevents compiler optimization
    }
}

//****************************************************************************
//
// GPIO initialization
//
//****************************************************************************

/************************************************************************************//**
 *
 * @brief InitGPIO()
 *          Configures the MCU's GPIO pins that interface with the ADC.
 *
 * @return none
 *
 */
void InitGPIO(void)
{
    // GPIO initialization is typically done in main.c by STM32CubeMX
    // But we can ensure the pins are configured correctly here
    
    // Set RESET pin high initially
    HAL_GPIO_WritePin(RESET_PORT, RESET_PIN, GPIO_PIN_SET);
    
    // Set START pin low initially  
    HAL_GPIO_WritePin(START_PORT, START_PIN, GPIO_PIN_RESET);
    
}


//****************************************************************************
//
// SPI Communication
//
//****************************************************************************

/************************************************************************************//**
 *
 * @brief InitSPI()
 *          Configures the MCU's SPI peripheral, for interfacing with the ADC.
 *
 * @return None
 */
void InitSPI(void)
{
    // SPI is initialized in main.c by STM32CubeMX
    // ADS124S08 requires SPI mode 1 (CPOL = 0, CPHA = 1)
    // This should be configured in the STM32CubeMX SPI settings
    
    // Verify SPI is ready
    if (hspi1.State != HAL_SPI_STATE_READY)
    {
        // Re-initialize if needed
        if (HAL_SPI_Init(&hspi1) != HAL_OK)
        {
            Error_Handler();
        }
    }
}

/************************************************************************************//**
 *
 * @brief InitADCPeripherals()
 *          Initialize MCU peripherals and pins to interface with ADC
 *
 * @param[in]   hspi    SPI_HandleTypeDef pointer for STM32 HAL
 *
 * @return      true for successful initialization
 *              false for unsuccessful initialization
 */
bool InitADCPeripherals( SPI_HandleTypeDef *hspi )
{
    bool status;

    // Initialize GPIO pins
    //InitGPIO();
    
    // Initialize SPI
    //InitSPI();
    
    // Verify SPI handle is valid
    if (hspi == NULL || hspi->State != HAL_SPI_STATE_READY) 
    {
        return false;
    }

    // Start up the ADC
    status = adcStartupRoutine( hspi );

    return status;
}


/************************************************************************************//**
 *
 * @brief getRESET()
 *          Returns the state of the MCU's ADC_RESET GPIO pin
 *
 * @return boolean level of /RESET pin (false = low, true = high)
 */
bool getRESET( void )
{
    return (HAL_GPIO_ReadPin(RESET_PORT, RESET_PIN) == GPIO_PIN_SET) ? true : false;
}

/************************************************************************************//**
 *
 * @brief setRESET()
 *            Sets the state of the MCU ADC_RESET GPIO pin
 *
 * @param[in]   state   level of /RESET pin (false = low, true = high)
 *
 * @return      None
 */

void setRESET( bool state )
{
    HAL_GPIO_WritePin(RESET_PORT, RESET_PIN, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/************************************************************************************//**
 *
 * @brief toggleRESET()
 *            Pulses the /RESET GPIO pin low
 *
 * @return      None
 */
void toggleRESET( void )
{
    HAL_GPIO_WritePin(RESET_PORT, RESET_PIN, GPIO_PIN_RESET);

    // Minimum nRESET width: 4 tCLKs = 4 * 1/4.096MHz =
    delay_us( DELAY_4TCLK );

    HAL_GPIO_WritePin(RESET_PORT, RESET_PIN, GPIO_PIN_SET);
}

/************************************************************************************//**
 *
 * @brief getSTART()
 *          Returns the state of the MCU's ADC_START GPIO pin
 *
 * @return boolean level of START pin (false = low, true = high)
 */
bool getSTART( void )
{
    return (HAL_GPIO_ReadPin(START_PORT, START_PIN) == GPIO_PIN_SET) ? true : false;
}

/************************************************************************************//**
 *
 * @brief setSTART()
 *            Sets the state of the MCU START GPIO pin
 *
 * @param[in]   state   level of START pin (false = low, true = high)
 *
 * @return      None
 */
void setSTART( bool state )
{
    HAL_GPIO_WritePin(START_PORT, START_PIN, state ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // Minimum START width: 4 tCLKs
    delay_us( DELAY_4TCLK );
}

/************************************************************************************//**
 *
 * @brief toggleSTART()
 *            Pulses the START GPIO pin low
 * param[in]    direction sets the toggle direction base on initial START pin configuration
 *
 * @return      None
 */
void toggleSTART( bool direction )
{
    if ( direction )
    {
        HAL_GPIO_WritePin(START_PORT, START_PIN, GPIO_PIN_RESET);

        // Minimum START width: 4 tCLKs
        delay_us( DELAY_4TCLK );

        HAL_GPIO_WritePin(START_PORT, START_PIN, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(START_PORT, START_PIN, GPIO_PIN_SET);

        // Minimum START width: 4 tCLKs
        delay_us( DELAY_4TCLK );

        HAL_GPIO_WritePin(START_PORT, START_PIN, GPIO_PIN_RESET);
    }
}

/************************************************************************************//**
 *
 * @brief sendSTART()
 *            Sends START Command through SPI
 *
 * @param[in]   spiHdl    SPI_Handle pointer for TI Drivers
 *
 * @return      None
 */
void sendSTART( SPI_HandleTypeDef *hspi )
{
    uint8_t dataTx = OPCODE_START;

    // Send START Command
    sendCommand( hspi, dataTx );
}

/************************************************************************************//**
 *
 * @brief sendSTOP()
 *            Sends STOP Command through SPI
 *
 * @param[in]   hspi    SPI_HandleTypeDef pointer for STM32 HAL
 *
 * @return      None
 */
void sendSTOP( SPI_HandleTypeDef *hspi )
{
    uint8_t dataTx = OPCODE_STOP;

    // Send STOP Command
    sendCommand( hspi, dataTx );
}

/************************************************************************************//**
 *
 * @brief sendRESET()
 *            Sends RESET Command through SPI, then waits 4096 tCLKs
 *
 * @param[in]   hspi    SPI_HandleTypeDef pointer for STM32 HAL
 *
 * @return      None
 */
void sendRESET( SPI_HandleTypeDef *hspi )
{
    uint8_t dataTx = OPCODE_RESET;

    // Send RESET command
    sendCommand( hspi, dataTx );
}

/************************************************************************************//**
 *
 * @brief sendWakeup()
 *            Sends WAKEUP command through SPI
 *
 * @param[in]   hspi    SPI_HandleTypeDef pointer for STM32 HAL
 *
 * @return      None
 */
void sendWakeup( SPI_HandleTypeDef *hspi )
{
    uint8_t dataTx = OPCODE_WAKEUP;

    // Wakeup device
    sendCommand( hspi, dataTx );
}

/************************************************************************************//**
 *
 * @brief sendPowerdown()
 *            Sends POWERDOWN command through SPI
 *
 * @param[in]   hspi    SPI_HandleTypeDef pointer for STM32 HAL
 *
 * @return      None
 */
void sendPowerdown( SPI_HandleTypeDef *hspi )
{
    uint8_t dataTx = OPCODE_POWERDOWN;

    // Power down device
    sendCommand( hspi, dataTx );
}

/************************************************************************************//**
 *
 * @brief setCS()
 *            Sets the state of the "/CS" GPIO pin
 *
 * @param[in]   level   Sets the state of the "/CS" pin
 *
 * @return      None
 */
void setCS( bool state )
{
    // For STM32 SPI, CS is typically handled automatically
    // If manual CS control is needed, use a GPIO pin
    // Here we assume CS is controlled by the SPI peripheral
    // If you need manual control, uncomment the lines below and define CS_PORT and CS_PIN

    HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/************************************************************************************//**
 *
 * @brief getCS()
 *          Returns the state of the MCU's ADC_CS GPIO pin
 *
 * @return boolean level of CS pin (false = low, true = high)
 */
bool getCS( void )
{
    // For STM32 SPI, CS is typically handled automatically
    // If manual CS control is needed, use a GPIO pin
    // Here we assume CS is controlled by the SPI peripheral

    return (HAL_GPIO_ReadPin(SPI_CS_GPIO_Port, SPI_CS_Pin) == GPIO_PIN_SET) ? true : false;
    //return true; // Default return for automatic CS control
}

/************************************************************************************//**
 *
 * @brief waitForDRDYHtoL()
 *            Waits for a nDRDY GPIO to go from High to Low or until a timeout condition occurs
 *            The DRDY output line is used as a status signal to indicate
 *            when a conversion has been completed. DRDY goes low
 *            when new data is available.
 *
 * @param[in]   timeout_ms number of milliseconds to allow until a timeout
 *
 * @return      Returns true if nDRDY interrupt occurred before the timeout
 *
 * @code
 *      // Read next conversion result
 *      if ( waitForDRDYHtoL( TIMEOUT_COUNTER ) ) {
 *          adcValue = readConvertedData( spiHdl, &status, COMMAND );
 *      } else {
 *          // Error reading conversion result
 *          Display_printf( displayHdl, 0, 0, "Timeout on conversion\n" );
 *          return( false );
 *      }
 * @endcode
 */
bool waitForDRDYHtoL( uint32_t timeout_ms )
{
    // uint32_t timeoutCounter = timeout_ms * 8000;   // convert to # of loop iterations;

    // do {
    // } while ( !HAL_GPIO_ReadPin(ADC_RDY_GPIO_Port, ADC_RDY_Pin) && (--timeoutCounter) );

    // if ( !timeoutCounter ) {
    //     return false;
    // } else {
    //     return true;
    // }

    if (!HAL_GPIO_ReadPin(ADC_RDY_GPIO_Port, ADC_RDY_Pin)) {
        // DRDY is already low, no need to wait
        return true;
    } else {
        // Don't wait
        return false;
    }
}

/************************************************************************************//**
 *
 * @brief spiSendReceiveArrays()
 *             Sends SPI commands to ADC and returns a response in array format
 *
 * @param[in]   spiHdl      SPI_Handle from TI Drivers
 * @param[in]   *DataTx     array of SPI data to send on MOSI pin
 * @param[in]   *DataRx     array of SPI data that will be received from MISO pin
 * @param[in]   byteLength  number of bytes to send/receive on the SPI
 *
 * @return     None
 */
void spiSendReceiveArrays( SPI_HandleTypeDef *hspi, uint8_t DataTx[], uint8_t DataRx[], uint8_t byteLength )
{
    HAL_StatusTypeDef status;

    /*
     *  This function sends and receives multiple bytes over the SPI.
     *
     *  A typical SPI send/receive sequence:
     *  1) Set the /CS pin low (if controlled by GPIO)
     *  2) Send/Receive data via SPI
     *  3) Set the /CS pin high (if controlled by GPIO)
     *
     *  For STM32, CS is typically controlled automatically by the SPI peripheral
     */

    setCS(LOW);
    // Send and receive data simultaneously
    if ( byteLength > 0 ) {
        status = HAL_SPI_TransmitReceive(hspi, DataTx, DataRx, byteLength, HAL_MAX_DELAY);
        if (status != HAL_OK) {
            // Handle SPI error
            Error_Handler();
        }
    }
    setCS(HIGH);
}


/************************************************************************************//**
 *
 * @brief spiSendReceiveByte()
 *             Sends a single byte to ADC and returns a response
 *
 * @param[in]   hspi        SPI_HandleTypeDef for STM32 HAL
 * @param[in]   dataTx      byte to send on DIN pin
 *
 * @return     SPI response byte
 */
uint8_t spiSendReceiveByte( SPI_HandleTypeDef *hspi, uint8_t dataTx )
{
    uint8_t dataRx = 0;
    HAL_StatusTypeDef status;

    // Send and receive single byte
    setCS(LOW);
    status = HAL_SPI_TransmitReceive(hspi, &dataTx, &dataRx, 1, HAL_MAX_DELAY);
    setCS(HIGH);

    if (status != HAL_OK) {
        // Handle SPI error
        Error_Handler();
    }
    
    return dataRx;
}
