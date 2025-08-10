#include "stm32g0xx_hal.h"
#include "peripherals.h"

// Override the HAL_InitTick function
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    return HAL_OK;
}

// HAL_GetTick override
uint32_t HAL_GetTick(void)
{
    return __HAL_TIM_GET_COUNTER(&htim2);
}

// Optional: no-op for HAL_Delay to still work with interrupts disabled
void HAL_Delay(uint32_t Delay)
{
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < Delay)
    {
        // Optionally call __WFI() here to sleep
    }
}