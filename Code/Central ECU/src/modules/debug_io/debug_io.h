#ifndef DEBUG_IO_H
#define DEBUG_IO_H

// Define one of these:
// #define DEBUG_OUTPUT_UART
#define DEBUG_OUTPUT_USB

void dbg_printf(const char *fmt, ...);
int dbg_recv(char *buffer, int max_length);

#endif // DEBUG_IO_H