#include "pte7300.h"
#include "debug_io.h"

bool PTE7300_Init(PTE7300_HandleTypeDef *hpte7300, I2C_HandleTypeDef *hi2c, uint8_t SID) {
    hpte7300->hi2c = *hi2c;
    hpte7300->SID = SID;
    can_buffer_init(&hpte7300->buffer, SID, BUFF_SIZE_PT, true); // 28 as multiple of 2 for pressure and temperature samples

    // Start the PTE7300 Pressure Sensor
    uint8_t cmd[2];
    cmd[0] = (PTE7300_CMD_START >> 8) & 0xFF; // High byte
    cmd[1] = PTE7300_CMD_START & 0xFF; // Low byte
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hpte7300->hi2c, PTE7300_I2C_ADDR << 1, PTE7300_ADDR_CMD, I2C_MEMADD_SIZE_8BIT, cmd, sizeof(cmd), HAL_MAX_DELAY);
    if (status != HAL_OK) {
        dbg_printf("PTE7300 SID=%d: Failed to start sensor\r\n", SID);
        return false;
    }
    return true;
}

bool PTE7300_sample_to_buffer(PTE7300_HandleTypeDef *hpte7300) {

    // Read the pressure sample from the PTE7300 at register 0x30
    // PTE7300 uses 16-bit registers and sends low byte first, then high byte
    uint8_t reg = PTE7300_ADDR_DSP_S;
    uint8_t pressure_bytes[2];
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hpte7300->hi2c, PTE7300_I2C_ADDR << 1, reg, I2C_MEMADD_SIZE_8BIT, pressure_bytes, sizeof(pressure_bytes), HAL_MAX_DELAY);
    if (status != HAL_OK) {
        dbg_printf("PTE7300: Failed to read pressure sample\r\n");
        return false;
    }

    // Read the temperature sample from the PTE7300
    reg = PTE7300_ADDR_DSP_T;
    uint8_t temperature_bytes[2];
    status = HAL_I2C_Mem_Read(&hpte7300->hi2c, PTE7300_I2C_ADDR << 1, reg, I2C_MEMADD_SIZE_8BIT, temperature_bytes, sizeof(temperature_bytes), HAL_MAX_DELAY);
    if (status != HAL_OK) {
        dbg_printf("PTE7300: Failed to read temperature sample\r\n");
        return false;
    }

    // Combine bytes: PTE7300 sends low byte first, then high byte (little-endian)
    int16_t pressure = (int16_t)(pressure_bytes[1] << 8 | pressure_bytes[0]);
    int16_t temperature = (int16_t)(temperature_bytes[1] << 8 | temperature_bytes[0]);
    
    can_buffer_push(&hpte7300->buffer, pressure);
    can_buffer_push(&hpte7300->buffer, temperature);
    return true;
}
