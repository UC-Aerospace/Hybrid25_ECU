#include "rtc_helper.h"

void rtc_helper_set_datetime(RTC_TimeTypeDef *time, RTC_DateTypeDef *date) {
    // Set RTC time and date
    HAL_RTC_SetTime(&hrtc, time, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, date, RTC_FORMAT_BIN);
}

void rtc_helper_get_datetime(RTC_TimeTypeDef *time, RTC_DateTypeDef *date) {
    HAL_RTC_GetTime(&hrtc, time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, date, RTC_FORMAT_BIN);
}

void rtc_helper_set_from_string(const char *time_str) {
    int days, milliseconds;
    if (sscanf(time_str, "%d %d", &days, &milliseconds) != 2) {
        return;  // Invalid format
    }

    // Calculate total seconds since start of 2025
    uint32_t total_seconds = (days * 86400) + (milliseconds / 1000);
    
    // Calculate date components
    RTC_DateTypeDef date = {0};
    date.Year = 25;  // 2025
    date.Month = RTC_MONTH_JANUARY;
    date.Date = 1;
    date.WeekDay = RTC_WEEKDAY_WEDNESDAY;  // Jan 1, 2025 is a Wednesday
    
    // Add the days
    for (int i = 0; i < days; i++) {
        date.Date++;
        date.WeekDay = (date.WeekDay % 7) + 1;
        
        // Handle month transitions
        switch (date.Month) {
            case RTC_MONTH_JANUARY:
            case RTC_MONTH_MARCH:
            case RTC_MONTH_MAY:
            case RTC_MONTH_JULY:
            case RTC_MONTH_AUGUST:
            case RTC_MONTH_OCTOBER:
            case RTC_MONTH_DECEMBER:
                if (date.Date > 31) {
                    date.Date = 1;
                    date.Month++;
                }
                break;
            case RTC_MONTH_APRIL:
            case RTC_MONTH_JUNE:
            case RTC_MONTH_SEPTEMBER:
            case RTC_MONTH_NOVEMBER:
                if (date.Date > 30) {
                    date.Date = 1;
                    date.Month++;
                }
                break;
            case RTC_MONTH_FEBRUARY:
                // 2025 is not a leap year
                if (date.Date > 28) {
                    date.Date = 1;
                    date.Month++;
                }
                break;
        }
    }
    
    // Calculate time components from milliseconds
    RTC_TimeTypeDef time = {0};
    uint32_t seconds_today = milliseconds / 1000;
    time.SubSeconds = milliseconds % 1000;
    time.Hours = seconds_today / 3600;
    time.Minutes = (seconds_today % 3600) / 60;
    time.Seconds = seconds_today % 60;
    time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    time.StoreOperation = RTC_STOREOPERATION_RESET;
    
    // Set the RTC
    rtc_helper_set_datetime(&time, &date);
}