#include "rs422.h"

// Global buffers for DMA operations
RS422_TxBuffer_t tx_buffer = {0}; // Initialize circular buffer for transmission
RS422_RxBuffer_t rx_buffer = {0}; // Initialize circular buffer for reception

// UART handle pointer for callbacks
static UART_HandleTypeDef *rs422_uart_handle = NULL;

static inline uint8_t dlc_to_len(uint8_t dlc)
{
    // DLC→length LUT for CAN FD (ISO 11898-1). Same as used here for RS422
    static const uint8_t dlc2len[16] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64
    };
    return (dlc < 16) ? dlc2len[dlc] : 0;   // 0 (or clamp) on invalid DLC
}

static inline uint8_t len_to_dlc(uint8_t len)
{
    if (len <= 8)  return len;
    if (len <= 12) return 9;
    if (len <= 16) return 10;
    if (len <= 20) return 11;
    if (len <= 24) return 12;
    if (len <= 32) return 13;
    if (len <= 48) return 14;
    return 15; // 49..64 → 64 bytes
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t transferred)
{
    if (huart->Instance == USART1) {
        rs422_process_rx_dma(transferred);
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

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        extern void dbg_printf(const char* format, ...);
        dbg_printf("RS422 UART Error: 0x%08lX\r\n", huart->ErrorCode);
        
        // Clear error flags and restart DMA
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_NEF | 
                             UART_CLEAR_PEF | UART_CLEAR_FEF);
        
        // Restart RX DMA
        HAL_UART_Receive_DMA(huart, rx_buffer.buffer, RS422_RX_BUFFER_SIZE);
    }
}

bool rs422_init(UART_HandleTypeDef *huart)
{
    // Store UART handle for callbacks
    rs422_uart_handle = huart;
    
    // Initialize TX circular buffer
    tx_buffer.head = 0;
    tx_buffer.tail = 0;
    tx_buffer.is_busy = false;
    
    // Initialize RX circular buffer
    rx_buffer.write_pos = 0;
    rx_buffer.read_pos = 0;

    // Start continuous DMA reception
    HAL_StatusTypeDef status = HAL_UARTEx_ReceiveToIdle_DMA(huart, rx_buffer.buffer, RS422_RX_BUFFER_SIZE);
    
    // Debug output
    extern void dbg_printf(const char* format, ...);
    if (status == HAL_OK) {
        dbg_printf("RS422 init: DMA reception started successfully\r\n");
    } else {
        dbg_printf("RS422 init: DMA reception failed with status %d\r\n", status);
    }

    return (status == HAL_OK);
}

bool rs422_send(uint8_t *data, uint8_t size, RS422_FrameType_t frame_type)
{
    if (size > RS422_TX_MESSAGE_SIZE - 3) {
        return false; // Data too large for packet
    }
    
    // Check if there's space in the buffer
    if (rs422_get_tx_buffer_space() == 0) {
        return false; // Buffer full
    }

    uint8_t dlc = len_to_dlc(size); // Convert length to DLC format
    size = dlc_to_len(dlc); // Get actual size based on DLC

    // Compute Header
    uint8_t header = frame_type << 4; // Shift frame type to the upper nibble
    header |= size & 0x0F; // Set the lower nibble to the DLC

    // Compute CRC-16 using the HAL of header and data
    
    // Copy data to the next packet in the buffer
    memcpy(&tx_buffer.buffer[tx_buffer.tail].data[0], &header, 1); // Set header byte
    memcpy(&tx_buffer.buffer[tx_buffer.tail].data[1], data, size); // Copy data bytes

    tx_buffer.buffer[tx_buffer.tail].size = size;
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
    
    // Start DMA transfer for the next packet with actual data size
    tx_buffer.is_busy = true;
    HAL_UART_Transmit_DMA(rs422_uart_handle, 
                         tx_buffer.buffer[tx_buffer.head].data, 
                         tx_buffer.buffer[tx_buffer.head].size);
}

void rs422_process_rx_dma(uint16_t transferred)
{
    if (transferred != RS422_RX_BUFFER_SIZE) {
        rx_buffer.write_pos = transferred;
    } else {
        uint8_t length_code = dlc_to_len(rx_buffer.buffer[rx_buffer.read_pos] & 0x0F); // Compute length of packet
        if (rx_buffer.read_pos + length_code == RS422_RX_BUFFER_SIZE) {
           rx_buffer.write_pos = transferred;
        } else {
            // Do nothing here, we will get the actual final position in the next callback
        }
    }
    
}

uint16_t rs422_get_rx_available(void)
{
    uint16_t available = 0;
    
    // Calculate available bytes based on DMA position
    if (rx_buffer.write_pos >= rx_buffer.read_pos) {
        available = rx_buffer.write_pos - rx_buffer.read_pos;
    } else {
        available = RS422_RX_BUFFER_SIZE - rx_buffer.read_pos + rx_buffer.write_pos;
    }
    
    return available;
}

uint16_t rs422_read(uint8_t *data, uint16_t max_len)
{
    uint16_t available = rs422_get_rx_available();
    uint16_t to_read = (available < max_len) ? available : max_len;
    uint16_t bytes_read = 0;
    
    if (to_read == 0) {
        return 0;
    }
    
    // Read data from circular buffer
    while (bytes_read < to_read) {
        data[bytes_read] = rx_buffer.buffer[rx_buffer.read_pos];
        rx_buffer.read_pos = (rx_buffer.read_pos + 1) % RS422_RX_BUFFER_SIZE;
        bytes_read++;
    }
    
    return bytes_read;
}