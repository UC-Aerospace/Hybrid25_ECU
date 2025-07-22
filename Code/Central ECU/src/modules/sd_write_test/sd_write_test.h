#ifndef SD_WRITE_TEST_H
#define SD_WRITE_TEST_H

#include "stm32g0xx_hal.h"
#include "sd_log.h"
#include "ff.h" // FatFs header file

/**
 * @brief Test SD card write performance using sd_log raw write function
 * @param path File path (not used in current implementation as sd_log manages paths)
 * @param data Data string (not used in current implementation as we use fixed test data)
 * @return FRESULT status code
 */
FRESULT sd_write_test(const char* path, const char* data);

/**
 * @brief Run multiple SD write tests and calculate average performance
 * @param num_tests Number of test iterations to run
 * @return Average write time in milliseconds, or 0 if tests failed
 */
uint32_t sd_write_test_benchmark(uint32_t num_tests);

#endif /* SD_WRITE_TEST_H */