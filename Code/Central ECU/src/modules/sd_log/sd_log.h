#ifndef SD_LOG_H
#define SD_LOG_H

#include "stm32g0xx_hal.h"
#include "ff.h"
#include "peripherals.h"
#include "rtc_helper.h"
#include <stdbool.h>
#include <stdint.h>

// Maximum length for log messages
#define SD_LOG_MAX_MSG_LEN 256

// Log types
typedef enum {
    SD_LOG_RAW,
    SD_LOG_INFO,
    SD_LOG_ERROR,
    SD_LOG_CRASH
} SD_LogType_t;

// Initialize the SD logging system
// Returns true if successful, false otherwise
bool sd_log_init(void);

// Write raw data to the raw log file
// Returns true if successful, false otherwise
bool sd_log_write_raw(const uint8_t* data, uint32_t len);

// Write a formatted log message
// Returns true if successful, false otherwise
bool sd_log_write(SD_LogType_t type, const char* format, ...);

// Flush all pending writes to disk
// Should be called periodically to ensure data is written
void sd_log_flush(void);

// Preallocate space for the raw log file
// size: Number of bytes to preallocate
// Returns true if successful, false otherwise
bool sd_log_preallocate_raw(uint32_t size);

// Get the current log directory name
// Returns the directory name as a string
const char* sd_log_get_dir_name(void);

#endif // SD_LOG_H