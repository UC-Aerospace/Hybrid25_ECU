#include "rs422.h"

uint8_t rx_buff[10] = {0}; // Buffer for received data
bool rx_buff_ready = false;
RS422_TxBuffer_t tx_buffer = {0}; // Initialize circular buffer

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        // Process received data here if needed
        HAL_UART_Receive_IT(huart, rx_buff, sizeof(rx_buff));
        rx_buff_ready = true;
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        // Update buffer state after DMA transfer completes
        tx_buffer.head = (tx_buffer.head + 1) % RS422_TX_BUFFER_SIZE;
        tx_buffer.is_busy = false;
        
        // Process next packet if available
        rs422_process_tx_queue();
    }
}

bool rs422_init(UART_HandleTypeDef *huart)
{
    // Initialize circular buffer
    tx_buffer.head = 0;
    tx_buffer.tail = 0;
    tx_buffer.is_busy = false;
    
    // Start receiving data in interrupt mode
    HAL_StatusTypeDef status = HAL_UART_Receive_IT(huart, rx_buff, sizeof(rx_buff));
    
    // Debug output
    extern void dbg_printf(const char* format, ...);
    if (status == HAL_OK) {
        dbg_printf("RS422 init: UART receive started successfully\r\n");
    } else {
        dbg_printf("RS422 init: UART receive failed with status %d\r\n", status);
    }

    return (status == HAL_OK); // Return true only if initialization was successful
}

bool rs422_send(uint8_t *data, uint16_t size)
{
    if (size > sizeof(RS422_packet)) {
        return false; // Data too large for packet
    }
    
    // Check if there's space in the buffer
    if (rs422_get_tx_buffer_space() == 0) {
        return false; // Buffer full
    }
    
    // Copy data to the next packet in the buffer
    memcpy(tx_buffer.buffer[tx_buffer.tail], data, size);
    tx_buffer.tail = (tx_buffer.tail + 1) % RS422_TX_BUFFER_SIZE;
    
    // Start DMA transfer if not already in progress
    if (!tx_buffer.is_busy) {
        rs422_process_tx_queue();
    }
    
    return true;
}

uint16_t rs422_get_tx_buffer_space(void)
{
    return RS422_TX_BUFFER_SIZE - ((tx_buffer.tail - tx_buffer.head + RS422_TX_BUFFER_SIZE) % RS422_TX_BUFFER_SIZE);
}

void rs422_process_tx_queue(void)
{
    if (tx_buffer.is_busy) {
        return; // DMA transfer already in progress
    }
    
    // Check if there are packets to send
    if (tx_buffer.head == tx_buffer.tail) {
        return; // No packets to send
    }
    
    // Start DMA transfer for the next packet
    tx_buffer.is_busy = true; // Mark that we're sending one packet
    HAL_UART_Transmit_DMA(&huart1, tx_buffer.buffer[tx_buffer.head], sizeof(RS422_packet));
}

uint8_t* rs422_get_rx_data(void)
{
    if (rx_buff_ready) {
        rx_buff_ready = false; // Clear the ready flag
        return rx_buff;
    }
    return NULL; // No data ready
}