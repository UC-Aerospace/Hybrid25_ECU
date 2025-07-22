#include "sd_write_test.h"
#include "../debug_io/debug_io.h"
#include <string.h>
#include <stdio.h>

// Test data buffer - exactly 55 bytes
static const uint8_t test_data[55] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
    0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
    0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E,
    0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32,
    0x33, 0x34, 0x35, 0x36, 0x37
};

/**
 * @brief Test SD card write performance using sd_log raw write function
 * @param path File path (not used in current implementation as sd_log manages paths)
 * @param data Data string (not used in current implementation as we use fixed test data)
 * @return FRESULT status code
 */
FRESULT sd_write_test(const char* path, const char* data) {
    uint32_t start_tick, end_tick, duration_ms;
    bool result;
    
    // Get start timestamp
    start_tick = HAL_GetTick();
    
    // Write exactly 55 bytes using sd_log raw write function
    result = sd_log_write_raw(test_data, sizeof(test_data));
    
    // Get end timestamp
    end_tick = HAL_GetTick();
    
    // Calculate duration
    duration_ms = end_tick - start_tick;
    
    // Log the results
    if (result) {
        dbg_printf("SD Write Test: Successfully wrote %d bytes in %lu ms\n", 
                   sizeof(test_data), duration_ms);
        sd_log_write(SD_LOG_INFO, "SD Write Test: %d bytes written in %lu ms", 
                     sizeof(test_data), duration_ms);
        return FR_OK;
    } else {
        dbg_printf("SD Write Test: Failed to write data\n");
        sd_log_write(SD_LOG_ERROR, "SD Write Test: Failed to write %d bytes", sizeof(test_data));
        return FR_DISK_ERR;
    }
}

/**
 * @brief Run multiple SD write tests and calculate average performance
 * @param num_tests Number of test iterations to run
 * @return Average write time in milliseconds, or 0 if tests failed
 */
uint32_t sd_write_test_benchmark(uint32_t num_tests) {
    uint32_t total_time = 0;
    uint32_t successful_tests = 0;
    
    dbg_printf("Starting SD write benchmark with %lu tests...\n", num_tests);
    
    for (uint32_t i = 0; i < num_tests; i++) {
        uint32_t start_tick = HAL_GetTick();
        bool result = sd_log_write_raw(test_data, sizeof(test_data));
        uint32_t end_tick = HAL_GetTick();
        
        if (result) {
            total_time += (end_tick - start_tick);
            successful_tests++;
        }
        
        // Small delay between tests
        HAL_Delay(10);
    }
    
    if (successful_tests > 0) {
        uint32_t avg_time = total_time / successful_tests;
        dbg_printf("SD Write Benchmark: %lu/%lu tests passed, avg time: %lu ms\n", 
                   successful_tests, num_tests, avg_time);
        sd_log_write(SD_LOG_INFO, "SD Write Benchmark: %lu/%lu tests passed, avg: %lu ms", 
                     successful_tests, num_tests, avg_time);
        return avg_time;
    } else {
        dbg_printf("SD Write Benchmark: All tests failed\n");
        sd_log_write(SD_LOG_ERROR, "SD Write Benchmark: All %lu tests failed", num_tests);
        return 0;
    }
}
