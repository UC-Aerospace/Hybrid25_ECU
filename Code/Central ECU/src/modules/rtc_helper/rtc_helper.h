#ifndef RTC_HELPER_H
#define RTC_HELPER_H

#include "stm32g0xx_hal.h"
#include "peripherals.h"

void rtc_helper_set_datetime(RTC_TimeTypeDef *time, RTC_DateTypeDef *date);
void rtc_helper_get_datetime(RTC_TimeTypeDef *time, RTC_DateTypeDef *date);
void rtc_helper_set_from_string(const char *time_str);
uint8_t* rtc_helper_get_datetime_string(void);

#endif // RTC_HELPER_H