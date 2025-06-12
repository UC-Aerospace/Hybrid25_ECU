#include "rtc_helper.h"

void rtc_helper_init(void) {
    // Set the time to midnight on the 1st January 2025
    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};
    
    // Initialize RTC time
    time.Hours = 0;
    time.Minutes = 0;
    time.Seconds = 0;
    time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    time.StoreOperation = RTC_STOREOPERATION_RESET;
    
    // Initialize RTC date
    date.WeekDay = RTC_WEEKDAY_WEDNESDAY;
    date.Month = RTC_MONTH_JANUARY;
    date.Date = 1;
    date.Year = 25;  // 2025
    
    rtc_helper_set_datetime(&time, &date);
}

void rtc_helper_set_datetime(RTC_TimeTypeDef *time, RTC_DateTypeDef *date) {
    // Set RTC time and date
    HAL_RTC_SetTime(&hrtc, time, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, date, RTC_FORMAT_BIN);
}

void rtc_helper_get_datetime(RTC_TimeTypeDef *time, RTC_DateTypeDef *date) {
    HAL_RTC_GetTime(&hrtc, time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, date, RTC_FORMAT_BIN);
}