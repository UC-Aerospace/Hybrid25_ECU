#include "sd_log.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "debug_io.h"
#include "sdcard.h"

// File system objects
static FATFS fs;
static FIL log_file;        // log.txt (text)
static FIL sensors_file;    // sensors.raw (binary multiplexed sensor packets)

// Current directory name
static char current_dir[10];
static bool is_initialized = false;
static uint32_t dir_counter = 1;  // Counter for sequential directory numbering

// Buffer for formatted messages
static char msg_buffer[SD_LOG_MAX_MSG_LEN];

// ---------------- Buffered architecture -----------------
// We buffer textual log lines (including formatted messages) and binary
// sensor packets into RAM rings. Flushing is performed incrementally via
// sd_log_service(time_budget_ms) so that no single call blocks > ~time_budget.

// Debug/text log ring
static volatile uint16_t dbg_ring_head = 0;
static volatile uint16_t dbg_ring_tail = 0;
static char dbg_ring[SD_LOG_DEBUG_BUF_SIZE];

// Sensor binary ring
static uint8_t sens_ring[SD_LOG_SENS_BUF_SIZE];
static volatile uint16_t sens_head = 0;
static volatile uint16_t sens_tail = 0;

// Flush control flags/state
static volatile bool flush_logs_requested = false;
static volatile bool flush_sensors_requested = false;
static bool flush_logs_in_progress = false;
static bool flush_sensors_in_progress = false;

#ifndef SD_LOG_WRITE_CHUNK
#define SD_LOG_WRITE_CHUNK 256U
#endif

#ifndef MIN
#define MIN(a,b) (( (a) < (b) ) ? (a) : (b))
#endif

bool sd_preallocate_extra(FIL *file, uint32_t size);

// Ring helpers
static inline uint16_t dbg_ring_next(uint16_t idx)
{
    return (uint16_t)((idx + 1U) % SD_LOG_DEBUG_BUF_SIZE);
}

static inline bool dbg_ring_empty(void)
{
    return dbg_ring_head == dbg_ring_tail;
}

static inline void dbg_ring_push_char(char c)
{
    uint16_t next = dbg_ring_next(dbg_ring_head);
    if(next == dbg_ring_tail) { // overflow drop oldest
        dbg_ring_tail = dbg_ring_next(dbg_ring_tail);
    }
    dbg_ring[dbg_ring_head] = c;
    dbg_ring_head = next;
}

static inline uint16_t dbg_ring_pop_chunk(char *dst, uint16_t max_len)
{
    if(dbg_ring_empty()) return 0;

    if(dbg_ring_head >= dbg_ring_tail) {
        uint16_t avail = (uint16_t)(dbg_ring_head - dbg_ring_tail);
        if(avail > max_len) avail = max_len;
        memcpy(dst, &dbg_ring[dbg_ring_tail], avail);
        dbg_ring_tail = (uint16_t)(dbg_ring_tail + avail);
        return avail;
    } else {
        uint16_t to_end = (uint16_t)(SD_LOG_DEBUG_BUF_SIZE - dbg_ring_tail);
        if(to_end > max_len) to_end = max_len;
        memcpy(dst, &dbg_ring[dbg_ring_tail], to_end);
        dbg_ring_tail = (uint16_t)((dbg_ring_tail + to_end) % SD_LOG_DEBUG_BUF_SIZE);
        return to_end;
    }
}

static inline bool sens_empty(void)
{
    return sens_head == sens_tail;
}

static inline uint16_t sens_used(void)
{
    if (sens_head >= sens_tail) return (uint16_t)(sens_head - sens_tail);

    return (uint16_t)(SD_LOG_SENS_BUF_SIZE - sens_tail + sens_head);
}

static inline uint16_t sens_space(void)
{
    return (uint16_t)(SD_LOG_SENS_BUF_SIZE - sens_used() - 1U);
} 

static void sens_push(const uint8_t *data, uint16_t len)
{
    // Ensure space (drop oldest data if necessary to guarantee forward progress)
    if (sens_space() < len) {
        uint16_t to_free = (uint16_t)(len - sens_space());
        dbg_printf("!!WARN!! - Dropping %u bytes from sensor ring buffer\n", to_free);
        sens_tail = (uint16_t)((sens_tail + to_free) % SD_LOG_SENS_BUF_SIZE);
    }

    uint16_t first = (uint16_t)MIN(len, (uint16_t)(SD_LOG_SENS_BUF_SIZE - sens_head));
    memcpy(&sens_ring[sens_head], data, first);
    uint16_t rem = (uint16_t)(len - first);

    if(rem) memcpy(&sens_ring[0], data + first, rem);

    sens_head = (uint16_t)((sens_head + len) % SD_LOG_SENS_BUF_SIZE);
}

static uint16_t sens_peek(uint8_t *dst, uint16_t max_len)
{
    if(sens_empty()) return 0;

    if(sens_head >= sens_tail) {
        uint16_t avail = (uint16_t)(sens_head - sens_tail);
        if(avail > max_len) avail = max_len;
        memcpy(dst, &sens_ring[sens_tail], avail);
        return avail;

    } else {
        uint16_t to_end = (uint16_t)(SD_LOG_SENS_BUF_SIZE - sens_tail);
        if(to_end > max_len) to_end = max_len;
        memcpy(dst, &sens_ring[sens_tail], to_end);
        return to_end;

    }
}
static void sens_consume(uint16_t n)
{
    sens_tail = (uint16_t)((sens_tail + n) % SD_LOG_SENS_BUF_SIZE);
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
    
    // Create directory
    res = f_mkdir(current_dir);
    if (res != FR_OK && res != FR_EXIST) {
        return false;
    }
    
    // Create log.txt and sensors.raw
    if (!open_file(&log_file, "log.txt", FA_CREATE_ALWAYS | FA_WRITE)) {
        return false;
    }
    if (!open_file(&sensors_file, "sensors.raw", FA_CREATE_ALWAYS | FA_WRITE)) {
        return false;
    }

    // Preallocate 2Mb to log.txt and 10Mb to sensors.raw using sd_preallocate_extra
    sd_preallocate_extra(&log_file, 2 * 1024 * 1024);
    sd_preallocate_extra(&sensors_file, 10 * 1024 * 1024);
    // TODO: During write check extra space remaining and allocate more if neccessary. At 5kB/s, 10Mb should be good for 33 minutes.

    is_initialized = true;
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
    
    (void)res; (void)bytes_written; (void)target_file; (void)type;
    // Enqueue into debug ring
    const char *p = msg_buffer;
    while (*p) dbg_ring_push_char(*p++);
    flush_logs_requested = true;
    return true;
}

// ----- New non-blocking flush API -----
void sd_log_request_flush_logs(void)
{
    flush_logs_requested = true;
}

void sd_log_request_flush_sensors(void)
{
    flush_sensors_requested = true;
}

static bool flush_logs_step(uint32_t *budget_ms)
{
    if (!flush_logs_requested && !flush_logs_in_progress) return true;

    flush_logs_in_progress = true;
    uint32_t start = HAL_GetTick();
    char chunk[SD_LOG_WRITE_CHUNK];
    UINT written;

    while (*budget_ms > 0 && !dbg_ring_empty()) {
        uint16_t n = dbg_ring_pop_chunk(chunk, sizeof(chunk));
        if(n == 0) break;
        (void)f_write(&log_file, chunk, n, &written); // ignore errors
        if((HAL_GetTick() - start) >= *budget_ms) break;
    }

    if(dbg_ring_empty()) {
        (void)f_sync(&log_file);
        flush_logs_requested = false;
        flush_logs_in_progress = false;
    }

    uint32_t elapsed = HAL_GetTick() - start;
    if(elapsed >= *budget_ms) *budget_ms = 0; else *budget_ms -= elapsed;

    return !flush_logs_in_progress;
}

static bool flush_sensors_step(uint32_t *budget_ms){
    if(!flush_sensors_requested && !flush_sensors_in_progress) return true;
    flush_sensors_in_progress = true;
    uint32_t start = HAL_GetTick();
    uint8_t chunk[SD_LOG_WRITE_CHUNK];
    UINT written;
    while(*budget_ms > 0 && !sens_empty()){
        uint16_t n = sens_peek(chunk, sizeof(chunk));
        if(n == 0) break;
        (void)f_write(&sensors_file, chunk, n, &written); // ignore errors
        sens_consume(n);
        if((HAL_GetTick() - start) >= *budget_ms) break;
    }
    if(sens_empty()){
        (void)f_sync(&sensors_file);
        flush_sensors_requested = false;
        flush_sensors_in_progress = false;
    }
    uint32_t elapsed = HAL_GetTick() - start;
    if(elapsed >= *budget_ms) *budget_ms = 0; else *budget_ms -= elapsed;
    return !flush_sensors_in_progress;
}

bool sd_log_service(uint32_t time_budget_ms){
    if(!is_initialized) return true;
    // Simple ordering: logs then sensors
    (void)flush_logs_step(&time_budget_ms);
    if(time_budget_ms > 0) (void)flush_sensors_step(&time_budget_ms);
    return !(flush_logs_in_progress || flush_sensors_in_progress);
}

bool sd_log_flush_blocking(uint32_t timeout_ms){
    uint32_t start = HAL_GetTick();
    while(!sd_log_service(10)){
        if((HAL_GetTick() - start) > timeout_ms) return false;
    }
    return true;
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
    (void)sensor_ids; (void)count; // now a no-op
    return true;    
}

bool sd_log_write_sensor_chunk(CAN_ADCFrame* frame, uint8_t length) {
    if (!is_initialized) return false;
    if (frame == NULL) return false;
    uint8_t marker[5] = {0x00, 0x00, 0x00, 0x00, 0xA1};
    sens_push(marker, sizeof(marker));
    sens_push((uint8_t*)frame, length);
    flush_sensors_requested = true;
    return true;
}

void sd_log_capture_debug(const char *text) {
    if (text == NULL) return;
    const char *p = text;
    while(*p) dbg_ring_push_char(*p++);
    flush_logs_requested = true;
}

// Optional helper to preallocate extra space in sensors.raw
bool sd_log_preallocate_sensors(uint32_t size) {
    if(!is_initialized) return false;
    return sd_preallocate_extra(&sensors_file, size);
}

// Optional helper to preallocate extra space in log.txt
bool sd_log_preallocate_log(uint32_t size) {
    if(!is_initialized) return false;
    return sd_preallocate_extra(&log_file, size);
}