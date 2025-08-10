#ifndef DEBUG_IO_H
#define DEBUG_IO_H

#include "stm32g0xx_hal.h"
// Define one of these:
// #define DEBUG_OUTPUT_UART
#define DEBUG_OUTPUT_USB

void dbg_printf(const char *fmt, ...);
// Non-blocking: returns length of next complete line (without CR/LF), 0 if none, -1 on error
int dbg_recv(char *buffer, int max_length);

#ifdef DEBUG_OUTPUT_USB
void debug_io_usb_receive(uint8_t *buf, uint32_t len);
#endif

#endif // DEBUG_IO_H