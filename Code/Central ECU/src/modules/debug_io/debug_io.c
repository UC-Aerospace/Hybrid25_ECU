#include "debug_io.h"
#include "peripherals.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define DBG_BUF_SIZE 256
static char dbg_buf[DBG_BUF_SIZE];

#ifdef DEBUG_OUTPUT_USB
#include "usbd_cdc_if.h"
extern USBD_HandleTypeDef hUsbDeviceFS;
#endif

void dbg_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(dbg_buf, sizeof(dbg_buf), fmt, args);
    va_end(args);

#ifdef DEBUG_OUTPUT_UART
    // Adjust 'huart2' to your specific UART handle
    HAL_UART_Transmit(&huart2, (uint8_t *)dbg_buf, strlen(dbg_buf), HAL_MAX_DELAY);
#endif

#ifdef DEBUG_OUTPUT_USB
    if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) {
        USBD_CDC_SetTxBuffer(&hUsbDeviceFS, (uint8_t*)dbg_buf, strlen(dbg_buf));
        USBD_CDC_TransmitPacket(&hUsbDeviceFS);
    }
#endif
}
