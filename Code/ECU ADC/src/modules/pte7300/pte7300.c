#include "pte7300.h"
#include "debug_io.h"

void PTE7300_Init(PTE7300_HandleTypeDef *hpte7300, I2C_HandleTypeDef *hi2c, uint8_t SID) {
    hpte7300->hi2c = *hi2c;
    hpte7300->SID = SID;
    can_buffer_init(&hpte7300->buffer, SID, 28); // 28 as multiple of 2 for pressure and temperature samples

    // Start the PTE7300 Pressure Sensor
    uint8_t cmd[2];
    cmd[0] = (PTE7300_CMD_START >> 8) & 0xFF; // High byte
    cmd[1] = PTE7300_CMD_START & 0xFF; // Low byte
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hpte7300->hi2c, PTE7300_I2C_ADDR << 1, cmd, sizeof(cmd), HAL_MAX_DELAY);
    if (status != HAL_OK) {
        dbg_printf("PTE7300 SID=%d: Failed to start sensor\r\n", SID);
        return;
    }
}

void PTE7300_sample_to_buffer(PTE7300_HandleTypeDef *hpte7300) {

    // Read the pressure sample from the PTE7300 at register 0x30
    uint8_t reg = PTE7300_ADDR_DSP_S;
    uint8_t pressure[2];
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hpte7300->hi2c, PTE7300_I2C_ADDR << 1, reg, I2C_MEMADD_SIZE_8BIT, pressure, sizeof(pressure), HAL_MAX_DELAY);
    if (status != HAL_OK) {
        dbg_printf("PTE7300: Failed to read pressure sample\r\n");
        return;
    }

    // Read the temperature sample from the PTE7300
    reg = PTE7300_ADDR_DSP_T;
    uint8_t temperature[2];
    status = HAL_I2C_Mem_Read(&hpte7300->hi2c, PTE7300_I2C_ADDR << 1, reg, I2C_MEMADD_SIZE_8BIT, temperature, sizeof(temperature), HAL_MAX_DELAY);
    if (status != HAL_OK) {
        dbg_printf("PTE7300: Failed to read temperature sample\r\n");
        return;
    }

    can_buffer_push(&hpte7300->buffer, pressure[0] << 8 | pressure[1]); // Combine high and low byte into a single value
    can_buffer_push(&hpte7300->buffer, temperature[0] << 8 | temperature[1]); // Combine high and low byte into a single value
}
