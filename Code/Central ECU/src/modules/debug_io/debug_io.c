#include "debug_io.h"
#include "peripherals.h"
#include "sd_log.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define DBG_BUF_SIZE 256
static char dbg_buf[DBG_BUF_SIZE];

#ifndef DEBUG_RX_BUF_SIZE
#define DEBUG_RX_BUF_SIZE 256
#endif
static volatile uint8_t rx_ring[DEBUG_RX_BUF_SIZE];
static volatile uint16_t rx_head = 0; // write index
static volatile uint16_t rx_tail = 0; // read index

static void rx_push(uint8_t b) {
    uint16_t next = (uint16_t)((rx_head + 1) % DEBUG_RX_BUF_SIZE);
    if(next == rx_tail) { // overflow, drop oldest
        rx_tail = (uint16_t)((rx_tail + 1) % DEBUG_RX_BUF_SIZE);
    }
    rx_ring[rx_head] = b;
    rx_head = next;
}

static int rx_pop_line(char *dst, int max_len) {
    if(rx_head == rx_tail) return 0; // empty
    int count = 0;
    uint16_t idx = rx_tail;
    int found_nl = 0;
    while(idx != rx_head && count < (max_len-1)) {
        char c = rx_ring[idx];
        if(c == '\r' || c == '\n') { found_nl = 1; idx = (uint16_t)((idx + 1) % DEBUG_RX_BUF_SIZE); break; }
        dst[count++] = c;
        idx = (uint16_t)((idx + 1) % DEBUG_RX_BUF_SIZE);
    }
    if(!found_nl) return 0; // no full line yet
    rx_tail = idx; // consume incl newline
    dst[count] = '\0';
    return count;
}

#ifdef DEBUG_OUTPUT_UART
static uint8_t uart_rx_byte;
#endif

#ifdef DEBUG_OUTPUT_USB
#include "usbd_cdc_if.h"
extern USBD_HandleTypeDef hUsbDeviceFS;
#endif

void dbg_printf(const char *fmt, ...)
{
    // Obtain timestamp first to minimize time skew before message formatting
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;
    rtc_helper_get_datetime(&time, &date);

    // Build prefix then append formatted message in one pass to avoid extra buffers
    size_t pos = (size_t)snprintf(dbg_buf, DBG_BUF_SIZE,
                                  "[%02u:%02u:%02u.%03lu] ",
                                  (unsigned)time.Hours,
                                  (unsigned)time.Minutes,
                                  (unsigned)time.Seconds,
                                  (uint32_t)((time.SecondFraction - time.SubSeconds) * 1000u / (time.SecondFraction + 1u)));
    if(pos >= DBG_BUF_SIZE) pos = DBG_BUF_SIZE - 1; // safety

    va_list args;
    va_start(args, fmt);
    vsnprintf(dbg_buf + pos, DBG_BUF_SIZE - pos, fmt, args);
    va_end(args);

    // Ensure line termination (optional). Uncomment if you always want CRLF.
    // size_t len_now = strlen(dbg_buf);
    // if(len_now && dbg_buf[len_now-1] != '\n') {
    //     if(len_now < DBG_BUF_SIZE - 2) {
    //         dbg_buf[len_now++] = '\r';
    //         dbg_buf[len_now++] = '\n';
    //         dbg_buf[len_now] = '\0';
    //     }
    // }

    // Capture to SD log ring buffer (non-blocking)
    sd_log_capture_debug(dbg_buf);

#ifdef DEBUG_OUTPUT_UART
    HAL_UART_Transmit(&huart2, (uint8_t *)dbg_buf, strlen(dbg_buf), HAL_MAX_DELAY);
#endif

#ifdef DEBUG_OUTPUT_USB
    if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) {
        USBD_CDC_SetTxBuffer(&hUsbDeviceFS, (uint8_t*)dbg_buf, strlen(dbg_buf));
        USBD_CDC_TransmitPacket(&hUsbDeviceFS);
    }
#endif
}

// Non-blocking receive: returns length of next complete line (without CR/LF), 0 if none, -1 on error
int dbg_recv(char *buffer, int max_length)
{
    if (buffer == NULL || max_length <= 1) return -1;

#ifdef DEBUG_OUTPUT_UART
    // Ensure UART IT reception armed
    if(HAL_UART_GetState(&huart2) == HAL_UART_STATE_READY) {
        HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
    }
#endif
    return rx_pop_line(buffer, max_length);
}

#ifdef DEBUG_OUTPUT_UART
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if(huart == &huart2) {
        rx_push(uart_rx_byte);
        HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
    }
}
#endif

#ifdef DEBUG_OUTPUT_USB
// Expect user to modify CDC_Receive_FS to call this helper
void debug_io_usb_receive(uint8_t *buf, uint32_t len) {
    for(uint32_t i=0;i<len;i++) rx_push(buf[i]);
}
#endif


