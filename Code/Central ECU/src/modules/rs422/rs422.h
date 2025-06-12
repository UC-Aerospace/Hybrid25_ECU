#ifndef RS422_H
#define RS422_H

#include "stm32g0xx_hal.h"
#include "peripherals.h"
#include <stdbool.h>
#include <stdint.h>

// Circular buffer size for DMA transmission
#define RS422_TX_BUFFER_SIZE 8
#define RS422_TX_MESSAGE_SIZE 64

// Structure for RS422 circular buffer

typedef uint8_t RS422_packet[RS422_TX_MESSAGE_SIZE];

typedef struct {
    RS422_packet buffer[RS422_TX_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile bool is_busy;
} RS422_TxBuffer_t;

// Function declarations
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
bool rs422_init(UART_HandleTypeDef *huart);
bool rs422_send(uint8_t *data, uint16_t size);
uint16_t rs422_get_tx_buffer_space(void);
void rs422_process_tx_queue(void);

// External declarations
extern uint8_t rx_buff[10];  // Buffer for received data
extern RS422_TxBuffer_t tx_buffer;  // Circular buffer for transmission

#endif // RS422_H