#ifndef RS422_H
#define RS422_H

#include "stm32g0xx_hal.h"
#include "peripherals.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// Buffer configurations for optimal DMA performance
#define RS422_TX_BUFFER_SIZE 8
#define RS422_TX_MESSAGE_SIZE 67
#define RS422_RX_BUFFER_SIZE 335  // Double buffer for continuous DMA reception

typedef enum {
    RS422_FRAME_HEARTBEAT = 0b0000,
    RS422_FRAME_SWITCH_CHANGE = 0b0001,
    RS422_FRAME_VALVE_UPDATE = 0b0010,
    RS422_FRAME_LED_UPDATE = 0b0011,
    RS422_BATTERY_VOLTAGE_FRAME = 0b0100,
    // 0b0101
    RS422_FRAME_PRESSURE = 0b0110,
    RS422_FRAME_PRESSURE_TEMPERATURE = 0b0111,
    RS422_FRAME_TEMPERATURE = 0b1000,
    RS422_FRAME_LOAD_CELL = 0b1001,
    // 0b1010
    // 0b1011
    RS422_FRAME_COUNTDOWN = 0b1100,
    // 0b1101
    RS422_FRAME_ABORT = 0b1110,
    RS422_FRAME_FIRE = 0b1111
} RS422_FrameType_t;

// Structure for RS422 transmission circular buffer
typedef struct {
    uint8_t data[RS422_TX_MESSAGE_SIZE];
    uint16_t size;  // Actual size of data in this packet
} RS422_packet;

typedef struct {
    RS422_FrameType_t frame_type; // Type of RS422 frame
    uint8_t data[RS422_TX_MESSAGE_SIZE - 3]; // Data payload (excluding header and CRC)
    uint16_t size; // Total size of the packet (header + data + CRC)
} RS422_RxFrame_t;

typedef struct {
    RS422_packet buffer[RS422_TX_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile bool is_busy;
} RS422_TxBuffer_t;

// Structure for RS422 reception circular buffer with DMA
typedef struct {
    uint8_t buffer[RS422_RX_BUFFER_SIZE];
    volatile uint16_t write_pos;  // Current DMA write position
    volatile uint16_t read_pos;   // Current read position
} RS422_RxBuffer_t;

// Function declarations
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t transferred);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);
bool rs422_init(UART_HandleTypeDef *huart);
HAL_StatusTypeDef rs422_send(uint8_t *data, uint8_t size, RS422_FrameType_t frame_type);
uint16_t rs422_get_tx_buffer_space(void);
HAL_StatusTypeDef rs422_process_tx_queue(void);
uint16_t rs422_get_rx_available(void);
uint16_t rs422_read(RS422_RxFrame_t *frame);
void rs422_process_rx_dma(uint16_t transferred);
bool rs422_send_valve_position(uint8_t valve_pos);

#endif // RS422_H