#include "spicy.h"
#include "debug_io.h"

ADC_ChannelConfTypeDef ADC_IMON_Config = {
    .Channel = ADC_CHANNEL_15, // PB15
    .Rank = ADC_REGULAR_RANK_1,
    .SamplingTime = ADC_SAMPLETIME_3CYCLES_5
};

void spicy_init(void)
{
    return;
}

bool spicy_checks(void)
{
    // TODO: Just do basic checks here, states and such should be confirmed before getting here

    // Check if ESTOP (interlock) is not pressed
    if (!comp_get_interlock()) {
        dbg_printf("SPICY: Interlock not engaged\n");
        return false;
    }
    return true;
}

// SOFT ARM COMMANDS

bool spicy_arm(void)
{
    if (spicy_checks()) {
        HAL_GPIO_WritePin(ARM_HS_GPIO_Port, ARM_HS_Pin, GPIO_PIN_SET);
        dbg_printf("SPICY: Arm Set\n");
        return true;
    } else {
        dbg_printf("SPICY: Arm Fail\n");
        return false;
    }
}

bool spicy_disarm(void)
{
    HAL_GPIO_WritePin(ARM_HS_GPIO_Port, ARM_HS_Pin, GPIO_PIN_RESET);
    dbg_printf("SPICY: Arm Cleared\n");
    return true;
}

bool spicy_get_arm(void)
{
    return HAL_GPIO_ReadPin(ARM_HS_GPIO_Port, ARM_HS_Pin);
}

// COMPARATOR GET COMMANDS

bool comp_get_interlock(void)
{
    return HAL_GPIO_ReadPin(INTERLOCK_GPIO_Port, INTERLOCK_Pin);
}

bool comp_get_oxlow(void)
{
    return HAL_GPIO_ReadPin(OX_LOWSIDE_SENSE_GPIO_Port, OX_LOWSIDE_SENSE_Pin);
}

bool comp_get_ematch1(void)
{
    return HAL_GPIO_ReadPin(EMATCH1_CONT_GPIO_Port, EMATCH1_CONT_Pin);
}

bool comp_get_ematch2(void)
{
    return HAL_GPIO_ReadPin(EMATCH2_CONT_GPIO_Port, EMATCH2_CONT_Pin);
}

// POWER MONITOR COMMANDS

bool hss_get_IMON(void)
{
    // The functionality of this is broken as the pin should be an analog input.
    // High if >2.3A
    // Low if  <1A
    // Indeterminate if 1A-2.3A
    return HAL_GPIO_ReadPin(IMON_HS_GPIO_Port, IMON_HS_Pin);
}

bool hss_get_PGOOD(void)
{
    return HAL_GPIO_ReadPin(PGOOD_HS_GPIO_Port, PGOOD_HS_Pin);
}

// FIRE COMMANDS

bool spicy_open_solenoid(void) 
{
    if (spicy_checks()) {
        HAL_GPIO_WritePin(OX_FIRE_GPIO_Port, OX_FIRE_Pin, GPIO_PIN_SET);
        dbg_printf("SPICY: Opened Solenoid\n");
        return true;
    } else {
        dbg_printf("SPICY: Open Solenoid Check Fail\n");
        return false;
    }
}

bool spicy_close_solenoid(void)
{
    HAL_GPIO_WritePin(OX_FIRE_GPIO_Port, OX_FIRE_Pin, GPIO_PIN_RESET);
    dbg_printf("SPICY: Closed Solenoid\n");
    return true;
}

bool spicy_get_solenoid(void)
{
    return HAL_GPIO_ReadPin(OX_FIRE_GPIO_Port, OX_FIRE_Pin);
}

bool spicy_fire_ematch1(void)
{
    if (spicy_checks()) {
        HAL_GPIO_WritePin(EMATCH1_FIRE_GPIO_Port, EMATCH1_FIRE_Pin, GPIO_PIN_SET);
        dbg_printf("SPICY: Fired Ematch1\n");
        return true;
    } else {
        dbg_printf("SPICY: Fire Ematch1 Checks Fail\n");
        return false;
    }
}

bool spicy_off_ematch1(void)
{
    HAL_GPIO_WritePin(EMATCH1_FIRE_GPIO_Port, EMATCH1_FIRE_Pin, GPIO_PIN_RESET);
    dbg_printf("SPICY: Ematch1 Off\n");
    return true;
}

bool spicy_fire_ematch2(void)
{
    if (spicy_checks()) {
        HAL_GPIO_WritePin(EMATCH2_FIRE_GPIO_Port, EMATCH2_FIRE_Pin, GPIO_PIN_SET);
        dbg_printf("SPICY: Fired Ematch2\n");
        return true;
    } else {
        dbg_printf("SPICY: Fire Ematch2 Checks Fail\n");
        return false;
    }
}

bool spicy_off_ematch2(void)
{
    HAL_GPIO_WritePin(EMATCH2_FIRE_GPIO_Port, EMATCH2_FIRE_Pin, GPIO_PIN_RESET);
    dbg_printf("SPICY: Ematch2 Off\n");
    return true;
}