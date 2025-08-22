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

// Maximum number of sensor IDs (6-bit IDs => 64 max) (legacy, now we multiplex into one file)
#define SD_LOG_MAX_SENSORS 32

// Size of the internal non-blocking debug text ring buffer (bytes)
// Keep modest due to RAM constraints
#define SD_LOG_DEBUG_BUF_SIZE 2048
#define SD_LOG_SENS_BUF_SIZE 4096

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

// Request a flush of the textual log buffer (non-blocking; actual I/O done in service)
void sd_log_request_flush_logs(void);

// Request a flush of the sensors buffer (non-blocking; actual I/O done in service)
void sd_log_request_flush_sensors(void);

// Service routine that performs incremental flush work for both logs and sensors according
// to the provided time budget in milliseconds. Returns true when no pending work remains.
// Call from main loop / scheduler at a reasonably high rate (e.g. 100Hz+).
bool sd_log_service(uint32_t time_budget_ms);

// Blocking convenience flush (will internally time-slice to keep individual blocking
// intervals short < ~10ms). Returns true on success.
bool sd_log_flush_blocking(uint32_t timeout_ms);

// Preallocate space for the raw log file
// size: Number of bytes to preallocate
// Returns true if successful, false otherwise
bool sd_log_preallocate_raw(uint32_t size); // legacy (maps to sensors file)

// Get the current log directory name
// Returns the directory name as a string
const char* sd_log_get_dir_name(void);

// Configure which sensors should have dedicated files created under sensors/.
// Provide an array of 6-bit sensor IDs. Returns true on success.
// Legacy no-op retained for compatibility (now all sensors go to a single file); returns true.
bool sd_log_configure_sensors(const uint8_t *sensor_ids, uint8_t count);

// Append a binary sensor chunk to the per-sensor file with a simple delimited record:
// [0xA1][sampleRate(1)][timestamp(3)][len(2 LE)][payload(len)]
// Returns true on success. File is created on first write if not already opened.
bool sd_log_write_sensor_chunk(uint8_t sensor_id6, uint8_t sample_rate_bits,
                               const uint8_t *payload, uint16_t payload_len,
                               const uint8_t timestamp3[3]);

// Non-blocking capture of debug text. Safe to call from ISRs; it enqueues into an internal ring.
// The ring is drained and written to log.txt on sd_log_flush() or via sd_log_write().
void sd_log_capture_debug(const char *text);

#endif // SD_LOG_H