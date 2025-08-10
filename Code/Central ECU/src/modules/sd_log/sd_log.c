#include "sd_log.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "debug_io.h"
#include "sdcard.h"

// File system objects
static FATFS fs;
static FIL log_file;
static FIL crash_file;

// Sensor files: up to 32 5-bit IDs
static FIL sensor_files[SD_LOG_MAX_SENSORS];
static bool sensor_file_open[SD_LOG_MAX_SENSORS];
static bool sensor_configured[SD_LOG_MAX_SENSORS];

// Current directory name
static char current_dir[10];
static char sensors_dir[20];
static bool is_initialized = false;
static uint32_t dir_counter = 1;  // Counter for sequential directory numbering

// Buffer for formatted messages
static char msg_buffer[SD_LOG_MAX_MSG_LEN];

// Debug text capture ring buffer
static volatile uint16_t dbg_ring_head = 0;
static volatile uint16_t dbg_ring_tail = 0;
static char dbg_ring[SD_LOG_DEBUG_RING_SIZE];

static inline uint16_t dbg_ring_next(uint16_t idx) {
    return (uint16_t)((idx + 1U) % SD_LOG_DEBUG_RING_SIZE);
}

static inline bool dbg_ring_empty(void) {
    return dbg_ring_head == dbg_ring_tail;
}

static inline void dbg_ring_push_char(char c) {
    uint16_t next = dbg_ring_next(dbg_ring_head);
    if (next == dbg_ring_tail) {
        // overflow: drop oldest
        dbg_ring_tail = dbg_ring_next(dbg_ring_tail);
    }
    dbg_ring[dbg_ring_head] = c;
    dbg_ring_head = next;
}

static inline uint16_t dbg_ring_pop_chunk(char *dst, uint16_t max_len) {
    if (dbg_ring_empty()) return 0;
    uint16_t count = 0;
    // Pop up to the end of buffer or max_len
    if (dbg_ring_head >= dbg_ring_tail) {
        // contiguous region
        uint16_t avail = (uint16_t)(dbg_ring_head - dbg_ring_tail);
        if (avail > max_len) avail = max_len;
        memcpy(dst, &dbg_ring[dbg_ring_tail], avail);
        dbg_ring_tail = (uint16_t)(dbg_ring_tail + avail);
        count = avail;
    } else {
        // wrap-around; pop to end first
        uint16_t to_end = (uint16_t)(SD_LOG_DEBUG_RING_SIZE - dbg_ring_tail);
        if (to_end > max_len) to_end = max_len;
        memcpy(dst, &dbg_ring[dbg_ring_tail], to_end);
        dbg_ring_tail = (uint16_t)((dbg_ring_tail + to_end) % SD_LOG_DEBUG_RING_SIZE);
        count = to_end;
    }
    return count;
}

// Function to find the highest existing log number
static uint32_t find_highest_log_number(void) {
    FRESULT res;
    DIR dir;
    FILINFO fno;
    uint32_t highest = 0;
    
    res = f_opendir(&dir, "");
    if (res != FR_OK) {
        return 0;
    }
    
    while (1) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) {
            break;
        }
        
        // Check if this is a log directory (starts with "LOG_")
        if (fno.fattrib & AM_DIR && strncmp(fno.fname, "LOG_", 4) == 0) {
            // Convert the number part to integer
            uint32_t num = strtoul(fno.fname + 4, NULL, 10);
            if (num > highest) {
                highest = num;
            }
        }
    }
    
    f_closedir(&dir);
    return highest;
}

// Function to generate a unique directory name based on sequential numbering
static void generate_dir_name(void) {
    // Format: LOG_XXXXX where XXXXX is a sequential number
    snprintf(current_dir, sizeof(current_dir), "LOG_%04lu", dir_counter++);
}

static void build_sensors_dir_path(void) {
    snprintf(sensors_dir, sizeof(sensors_dir), "%s/sensors", current_dir);
}

// Function to create and open a file with error checking
static bool open_file(FIL* file, const char* filename, BYTE mode) {
    FRESULT res;
    char full_path[64];
    
    // Construct full path
    snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, filename);
    
    // Open file
    res = f_open(file, full_path, mode);
    if (res != FR_OK) {
        return false;
    }
    
    return true;
}

static bool open_file_path(FIL* file, const char* path, BYTE mode) {
    FRESULT res = f_open(file, path, mode);
    return res == FR_OK;
}

static void sensor_files_reset(void) {
    for (uint8_t i = 0; i < SD_LOG_MAX_SENSORS; i++) {
        sensor_file_open[i] = false;
        sensor_configured[i] = false;
    }
}

static bool open_sensor_file(uint8_t sid) {
    if (sid >= SD_LOG_MAX_SENSORS) return false;
    if (sensor_file_open[sid]) return true;
    char path[64];
    snprintf(path, sizeof(path), "%s/SID_%02X.raw", sensors_dir, sid & 0x3F);
    // Create always in new LOG_xxxx dir
    if (!open_file_path(&sensor_files[sid], path, FA_CREATE_ALWAYS | FA_WRITE)) {
        return false;
    }
    sensor_file_open[sid] = true;
    return true;
}

bool sd_log_init(void) {
    FRESULT res;
    
    // Mount the file system
    res = f_mount(&fs, "", 1);
    if (res != FR_OK) {
        dbg_printf("Error mounting file system: %d\n", res);
        return false;
    }
    
    // Find the highest existing log number and set our counter
    dir_counter = find_highest_log_number() + 1;
    
    // Generate unique directory name
    generate_dir_name();
    build_sensors_dir_path();
    
    // Create directory
    res = f_mkdir(current_dir);
    if (res != FR_OK && res != FR_EXIST) {
        return false;
    }

    // Create sensors subdirectory
    res = f_mkdir(sensors_dir);
    if (res != FR_OK && res != FR_EXIST) {
        return false;
    }
    
    // rename to log.txt per new structure
    if (!open_file(&log_file, "log.txt", FA_CREATE_ALWAYS | FA_WRITE)) {
        return false;
    }
    if (!open_file(&crash_file, "crash.txt", FA_CREATE_ALWAYS | FA_WRITE)) {
        f_close(&log_file);
        return false;
    }
    
    is_initialized = true;
    sensor_files_reset();

    sd_log_flush(); // Ensure all files are synced to disk initially
    return true;
}

bool sd_log_write(SD_LogType_t type, const char* format, ...) {
    va_list args;
    UINT bytes_written;
    FRESULT res;
    FIL* target_file;
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;
    
    if (!is_initialized || !format) {
        return false;
    }
    
    // Get current time from RTC
    rtc_helper_get_datetime(&time, &date);
    
    // Format timestamp
    snprintf(msg_buffer, sizeof(msg_buffer), "[%02d:%02d:%02d.%03lu] ",
             time.Hours, time.Minutes, time.Seconds, (uint32_t)((time.SecondFraction - time.SubSeconds) * 1000 / (time.SecondFraction + 1)));
    
    // Append the actual message
    va_start(args, format);
    vsnprintf(msg_buffer + strlen(msg_buffer),
              sizeof(msg_buffer) - strlen(msg_buffer),
              format, args);
    va_end(args);
    
    // Add newline
    strncat(msg_buffer, "\r\n", sizeof(msg_buffer) - strlen(msg_buffer) - 1);
    
    // Select target file based on log type
    switch (type) {
        case SD_LOG_INFO:
        case SD_LOG_ERROR:
            target_file = &log_file;
            break;
        case SD_LOG_CRASH:
            target_file = &crash_file;
            break;
        default:
            return false;
    }
    
    // Write to file
    res = f_write(target_file, msg_buffer, strlen(msg_buffer), &bytes_written);
    if (res != FR_OK || bytes_written != strlen(msg_buffer)) {
        return false;
    }
    
    // Sync to ensure data is written to disk
    //f_sync(target_file);
    
    return true;
}

void sd_log_flush(void) {
    if (!is_initialized) {
        return;
    }

    // Drain debug ring into log.txt in chunks to reduce write calls
    if (!dbg_ring_empty()) {
        char chunk[128];
        UINT written;
        while (!dbg_ring_empty()) {
            uint16_t n = dbg_ring_pop_chunk(chunk, sizeof(chunk));
            if (n == 0) break;
            (void)f_write(&log_file, chunk, n, &written);
            // If write fails, we just drop data to avoid blocking time-critical paths
        }
    }

    f_sync(&log_file);
    f_sync(&crash_file);

    // Also ensure any open sensor files are synced
    for (uint8_t i = 0; i < SD_LOG_MAX_SENSORS; i++) {
        if (sensor_file_open[i]) {
            f_sync(&sensor_files[i]);
        }
    }
}

bool sd_preallocate_extra(FIL *file, uint32_t size) {
    FRESULT res;
    FSIZE_t current_size;
    
    if (!is_initialized) {
        return false;
    }
    
    // Expand the file to the requested size
    current_size = f_size(file);
    size += current_size; // New size is current size + requested extra
    
    // Seek to the desired end position
    res = f_lseek(file, size - 1);
    if (res != FR_OK) {
        return false;
    }
    
    // Write a single byte to expand the file
    UINT bytes_written;
    uint8_t dummy_byte = 0;
    res = f_write(file, &dummy_byte, 1, &bytes_written);
    if (res != FR_OK || bytes_written != 1) {
        return false;
    }
    
    // Seek back to the original position (end of actual data)
    res = f_lseek(file, current_size);
    if (res != FR_OK) {
        return false;
    }
    
    // Sync to ensure allocation is committed to disk
    res = f_sync(file);
    if (res != FR_OK) {
        return false;
    }
    
    return true;
}

const char* sd_log_get_dir_name(void) {
    return current_dir;
}

bool sd_log_configure_sensors(const uint8_t *sensor_ids, uint8_t count) {
    if (!is_initialized) return false;
    if (sensor_ids == NULL && count != 0) return false;
    for (uint8_t i = 0; i < count; i++) {
        uint8_t sid = (uint8_t)(sensor_ids[i] & 0x3F);
        sensor_configured[sid] = true;
        if (!open_sensor_file(sid)) {
            return false;
        }
    }
    return true;
}

bool sd_log_write_sensor_chunk(uint8_t sensor_id6, uint8_t sample_rate_bits,
                               const uint8_t *payload, uint8_t payload_len,
                               const uint8_t timestamp3[3]) {
    if (!is_initialized) return false;
    if (payload == NULL && payload_len != 0) return false;
    uint8_t sid = (uint8_t)(sensor_id6 & 0x1F);
    if (!sensor_file_open[sid]) {
        // Open on first use or if pre-configured
        if (!open_sensor_file(sid)) {
            return false;
        }
    }
    // Build record header: 0xA1, sampleRate(1), timestamp(3), len(2 LE)
    uint8_t header[7];
    header[0] = 0xA1;
    header[1] = (uint8_t)(sample_rate_bits & 0x07);
    header[2] = timestamp3 ? timestamp3[0] : 0;
    header[3] = timestamp3 ? timestamp3[1] : 0;
    header[4] = timestamp3 ? timestamp3[2] : 0;
    header[5] = payload_len;

    UINT written;
    FRESULT r1 = f_write(&sensor_files[sid], header, sizeof(header), &written);
    if (r1 != FR_OK || written != sizeof(header)) return false;
    if (payload_len > 0) {
        r1 = f_write(&sensor_files[sid], payload, payload_len, &written);
        if (r1 != FR_OK || written != payload_len) return false;
    }
    return true;
}

void sd_log_capture_debug(const char *text) {
    if (text == NULL) return;
    // Fast path: push bytes into ring; ISR-safe with minimal critical section if needed
    // We avoid using dbg_printf here to prevent recursion
    const char *p = text;
    while (*p) {
        dbg_ring_push_char(*p++);
    }
}