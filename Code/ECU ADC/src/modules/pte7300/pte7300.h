#ifndef PTE7300_H
#define PTE7300_H

#include "stm32g0xx_hal.h"
#include "can_buffer.h"
#include "config.h"
#include <stdbool.h>

// PTE7300 ADC I2C address
#define PTE7300_I2C_ADDR       0x6C
// command register address
#define PTE7300_ADDR_CMD       0x22
// serial register
#define PTE7300_ADDR_SERIAL    0x50
// result registers
#define PTE7300_ADDR_DSP_T     0x2E
#define PTE7300_ADDR_DSP_S     0x30
#define PTE7300_ADDR_STATUS	   0x36
#define PTE7300_ADDR_ADC_TC    0x26
// define commands
#define PTE7300_CMD_START      0x8B93
#define PTE7300_CMD_SLEEP      0x6C32
#define PTE7300_CMD_RESET      0xB169
#define PTE7300_CMD_IDLE       0x7BBA 

typedef struct {
    I2C_HandleTypeDef hi2c;
    uint8_t SID;
    can_buffer_t buffer;
} PTE7300_HandleTypeDef;

bool PTE7300_Init(PTE7300_HandleTypeDef *hpte7300, I2C_HandleTypeDef *hi2c, uint8_t SID);
bool PTE7300_sample_to_buffer(PTE7300_HandleTypeDef *hpte7300);

#endif // PTE7300_H