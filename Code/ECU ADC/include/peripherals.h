#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include "stm32g0xx_hal.h"
#include "usb_device.h"

// Declare handles defined in main.c
extern ADC_HandleTypeDef hadc1;
extern FDCAN_HandleTypeDef hfdcan1;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern I2C_HandleTypeDef hi2c3;
extern RTC_HandleTypeDef hrtc;
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart2;
extern USBD_HandleTypeDef hUsbDeviceFS;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim14;
extern TIM_HandleTypeDef htim15;
extern DMA_HandleTypeDef hdma_adc1;
#endif // PERIPHERALS_H