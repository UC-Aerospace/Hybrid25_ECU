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
    #ifndef TEST_MODE
    return; // Disable debug output in non-test modes
    #endif
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

// Function to receive a string from either UART or USB CDC
// Returns number of bytes received, or -1 if error
int dbg_recv(char *buffer, int max_length)
{
    if (buffer == NULL || max_length <= 0) {
        return -1;
    }

    int bytes_received = 0;
    dbg_printf("Waiting for input...\r\n");

#ifdef DEBUG_OUTPUT_UART
    // Receive data from UART
    HAL_StatusTypeDef status = HAL_UART_Receive(&huart2, (uint8_t *)buffer, max_length - 1, HAL_MAX_DELAY);
    if (status == HAL_OK) {
        // Find the actual length of received data
        bytes_received = strlen(buffer);
        // Ensure null termination
        buffer[bytes_received] = '\0';
    } else {
        return -1;
    }
#endif

#ifdef DEBUG_OUTPUT_USB
    if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) {
        // Set up the receive buffer
        USBD_CDC_SetRxBuffer(&hUsbDeviceFS, (uint8_t*)buffer);
        
        // Start receiving data
        if (USBD_CDC_ReceivePacket(&hUsbDeviceFS) == USBD_OK) {
            // The actual data will be received in the CDC_Receive_FS callback
            // We need to wait for the data to be received
            // Note: This is a blocking implementation. For non-blocking,
            // you would need to implement a state machine or use a ring buffer
            while (hUsbDeviceFS.pClassDataCmsit[hUsbDeviceFS.classId] != NULL) {
                USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)hUsbDeviceFS.pClassDataCmsit[hUsbDeviceFS.classId];
                if (hcdc->RxLength > 0) {
                    bytes_received = hcdc->RxLength;
                    buffer[bytes_received] = '\0';  // Ensure null termination
                    hcdc->RxLength = 0;  // Reset for next reception
                    break;
                }
            }
        }
    }
#endif

    return bytes_received;
}


