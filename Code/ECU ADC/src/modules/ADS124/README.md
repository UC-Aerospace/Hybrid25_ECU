# ADS124S08 ADC Module

A comprehensive driver module for interfacing with the Texas Instruments ADS124S08 24-bit ADC for differential signal measurement in the Hybrid25 ECU project.

## Overview

This module provides a simple but flexible interface for the ADS124S08 8-channel, 24-bit analog-to-digital converter. It's designed specifically for measuring differential signals with configurable gain and sample rates, featuring dual buffering for continuous data acquisition and CAN bus transmission.

## Key Features

- **Differential Input Support**: Configure up to 12 differential channel pairs
- **Flexible Configuration**: Individual PGA gain (1x to 128x) and sample rate (2.5 to 4000 SPS) per channel
- **Dual Buffering**: 60-sample buffers with automatic switching for continuous operation
- **16-bit Decimation**: Converts 24-bit ADC data to 16-bit for efficient storage and transmission
- **CAN Integration**: Ready-to-use CAN transmission protocol for buffer data
- **Interrupt-Driven**: Uses data ready signal for efficient operation
- **Timestamp Recording**: System tick recorded at buffer start for time correlation

## File Structure

```
src/modules/ADS124/
├── ADS124.h                    # Main driver header
├── ADS124.c                    # Main driver implementation  
├── ADS124_can.h               # CAN transmission header
├── ADS124_can.c               # CAN transmission implementation
├── ADS124_example.c           # Usage examples
├── ADS124_config_guide.h      # Configuration guide
└── README.md                  # This file
```

## Hardware Configuration Required

### GPIO Pins
- **ADC_RDY_Pin (PC5)**: Data ready interrupt input (falling edge)
- **ADC_RST_Pin (PB0)**: Reset output to ADS124S08
- **SPI1 Pins**: Standard SPI communication

### SPI Configuration
The current SPI1 configuration needs modification:
```c
// Change this:
hspi1.Init.DataSize = SPI_DATASIZE_4BIT;

// To this:
hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
```

### Interrupt Configuration
Enable EXTI interrupt for ADC_RDY_Pin and add to the EXTI callback:
```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == ADC_RDY_Pin) {
        ads124_data_ready_callback(GPIO_Pin);
    }
}
```

## Basic Usage

### 1. Initialization
```c
#include "ADS124.h"

static ads124_handle_t ads124_handle;

void app_init(void) {
    // Initialize the ADS124 module
    if (ads124_init(&ads124_handle, &hspi1) != HAL_OK) {
        // Handle error
        return;
    }
    
    // Configure differential channels
    ads124_configure_channel(&ads124_handle, 0, 
                            ADS124_AIN0, ADS124_AIN1,      // AIN0(+) to AIN1(-)
                            ADS124_PGA_GAIN_32,             // 32x gain
                            ADS124_DATARATE_100SPS);        // 100 samples/second
    
    // Enable the channel
    ads124_enable_channel(&ads124_handle, 0, true);
    
    // Start continuous conversions
    ads124_start_conversion(&ads124_handle);
}
```

### 2. Data Processing
```c
void app_run(void) {
    while (1) {
        // Check for completed buffers
        ads124_data_buffer_t *buffer = ads124_get_completed_buffer(&ads124_handle);
        
        if (buffer != NULL) {
            // Buffer contains 60 samples ready for transmission
            // Timestamp: buffer->timestamp
            // Sample rate: buffer->sample_rate
            // Data: buffer->data[0] to buffer->data[59]
            
            // Transmit via CAN (see CAN integration below)
            ads124_can_transmit_buffer(&hfdcan1, buffer);
            
            // Mark buffer as processed
            ads124_release_buffer(&ads124_handle, buffer);
        }
        
        HAL_Delay(10);
    }
}
```

## Advanced Configuration

### Multiple Channels
```c
// Configure multiple differential channels with different settings
ads124_configure_channel(&ads124_handle, 0, ADS124_AIN0, ADS124_AIN1, ADS124_PGA_GAIN_64, ADS124_DATARATE_100SPS);
ads124_configure_channel(&ads124_handle, 1, ADS124_AIN2, ADS124_AIN3, ADS124_PGA_GAIN_16, ADS124_DATARATE_400SPS);
ads124_configure_channel(&ads124_handle, 2, ADS124_AIN4, ADS124_AIN5, ADS124_PGA_GAIN_4, ADS124_DATARATE_1000SPS);

// Enable channels
ads124_enable_channel(&ads124_handle, 0, true);
ads124_enable_channel(&ads124_handle, 1, true);
ads124_enable_channel(&ads124_handle, 2, true);
```

### Runtime Reconfiguration
```c
// Change channel configuration at runtime
ads124_stop_conversion(&ads124_handle);
ads124_configure_channel(&ads124_handle, 0, ADS124_AIN0, ADS124_AIN1, ADS124_PGA_GAIN_128, ADS124_DATARATE_50SPS);
ads124_start_conversion(&ads124_handle);
```

## CAN Integration

### Initialize CAN
```c
#include "ADS124_can.h"

// Initialize CAN for ADS124 data transmission
ads124_can_init(&hfdcan1);
```

### Transmit Buffer Data
```c
ads124_data_buffer_t *buffer = ads124_get_completed_buffer(&ads124_handle);
if (buffer != NULL) {
    // Transmits 1 header + 20 data messages
    ads124_can_transmit_buffer(&hfdcan1, buffer);
    ads124_release_buffer(&ads124_handle, buffer);
}
```

### CAN Message Protocol
- **Header Message (ID 0x100)**: Buffer metadata (timestamp, sample rate, buffer ID)
- **Data Messages (ID 0x101)**: Sample data (3 samples per message, 20 messages total)

## API Reference

### Core Functions
- `ads124_init()`: Initialize the ADS124 module
- `ads124_configure_channel()`: Configure differential input channel
- `ads124_enable_channel()`: Enable/disable channel
- `ads124_start_conversion()`: Start continuous conversion
- `ads124_stop_conversion()`: Stop conversion
- `ads124_get_completed_buffer()`: Get completed 60-sample buffer
- `ads124_release_buffer()`: Mark buffer as processed

### Utility Functions
- `ads124_convert_to_16bit()`: Convert 24-bit to 16-bit data
- `ads124_convert_to_voltage()`: Convert raw data to voltage
- `ads124_reset()`: Hardware reset of ADS124S08
- `ads124_read_id()`: Read device ID

## Data Buffer Structure

Each buffer contains:
```c
typedef struct {
    uint16_t data[60];        // 60 16-bit samples
    uint32_t timestamp;       // System tick at buffer start
    uint16_t sample_rate;     // Sample rate in Hz
    uint8_t buffer_id;        // Buffer identifier (0 or 1)
    bool ready;               // Buffer ready flag
} ads124_data_buffer_t;
```

## Channel Switching

The module automatically switches between enabled channels:
- Each channel can have different gain and sample rate settings
- Switching occurs after each conversion
- Only enabled channels are measured
- Round-robin scheduling through enabled channels

## Error Handling

- All functions return `HAL_StatusTypeDef` for error checking
- Invalid parameters return `HAL_ERROR`
- SPI communication errors are propagated
- Device ID verification during initialization

## Performance Considerations

- **Interrupt-driven**: Minimal CPU usage during conversion
- **Dual buffering**: Continuous operation without data loss
- **16-bit storage**: Reduced memory usage and CAN bandwidth
- **Configurable rates**: Optimize for your signal requirements

## Integration Checklist

1. ✅ Include ADS124 source files in build
2. ✅ Update SPI1 DataSize to 8-bit
3. ✅ Configure GPIO pins (ADC_RDY, ADC_RST)
4. ✅ Enable EXTI interrupt for data ready
5. ✅ Add interrupt callback
6. ✅ Initialize ADS124 in app_init()
7. ✅ Process buffers in main loop
8. ✅ Configure CAN for data transmission

## Future Enhancements

- **Calibration routines**: Offset and gain calibration
- **Filter configuration**: Digital filter settings
- **Power management**: Low-power modes
- **Error detection**: CRC checking, fault detection
- **Adaptive sampling**: Dynamic rate adjustment

## Questions and Support

For questions about the ADS124 module implementation, refer to:
- ADS124S08 datasheet for register details
- STM32G0 HAL documentation for peripheral configuration
- This README and example code for usage patterns

The module is designed to be simple but flexible - start with basic single-channel operation and expand as needed for your specific application requirements.
