#include "sd_log.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// File system objects
static FATFS fs;
static FIL raw_file;
static FIL log_file;
static FIL crash_file;

// Current directory name
static char current_dir[32];
static bool is_initialized = false;
static uint32_t dir_counter = 1;  // Counter for sequential directory numbering

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

// Buffer for formatted messages
static char msg_buffer[SD_LOG_MAX_MSG_LEN];

// Function to generate a unique directory name based on sequential numbering
static void generate_dir_name(void) {
    // Format: LOG_XXXXX where XXXXX is a sequential number
    snprintf(current_dir, sizeof(current_dir), "LOG_%05lu", dir_counter++);
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

bool sd_log_init(void) {
    FRESULT res;
    
    // Mount the file system
    res = f_mount(&fs, "", 1);
    if (res != FR_OK) {
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
    
    // Open all log files
    if (!open_file(&raw_file, "raw.txt", FA_CREATE_ALWAYS | FA_WRITE)) {
        return false;
    }
    if (!open_file(&log_file, "logs.txt", FA_CREATE_ALWAYS | FA_WRITE)) {
        f_close(&raw_file);
        return false;
    }
    if (!open_file(&crash_file, "crash.txt", FA_CREATE_ALWAYS | FA_WRITE)) {
        f_close(&raw_file);
        f_close(&log_file);
        return false;
    }
    
    is_initialized = true;
    return true;
}

bool sd_log_write_raw(const uint8_t* data, uint32_t len) {
    UINT bytes_written;
    FRESULT res;
    
    if (!is_initialized || !data || len == 0) {
        return false;
    }
    
    // Write data to raw file
    res = f_write(&raw_file, data, len, &bytes_written);
    if (res != FR_OK || bytes_written != len) {
        return false;
    }
    
    // Sync to ensure data is written to disk
    f_sync(&raw_file);
    
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
    snprintf(msg_buffer, sizeof(msg_buffer), "[%02d:%02d:%02d] ",
             time.Hours, time.Minutes, time.Seconds);
    
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
        case SD_LOG_RAW:
            target_file = &raw_file;
            break;
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
    f_sync(target_file);
    
    return true;
}

void sd_log_flush(void) {
    if (!is_initialized) {
        return;
    }
    
    f_sync(&raw_file);
    f_sync(&log_file);
    f_sync(&crash_file);
}

const char* sd_log_get_dir_name(void) {
    return current_dir;
}