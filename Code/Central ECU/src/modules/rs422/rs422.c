#include "rs422.h"
#include "crc.h"
#include "config.h"
#include "debug_io.h"
#include "spicy.h"
#include "sequencer.h"
#include "main_FSM.h"
#include "servo.h"

// Global buffers for DMA operations
RS422_TxBuffer_t tx_buffer = {0}; // Initialize circular buffer for transmission
static volatile RS422_RxBuffer_t rx_buffer = {0}; // Initialize circular buffer for reception

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
    if (HAL_UARTEx_GetRxEventType(huart) == HAL_UART_RXEVENT_IDLE) {
        rs422_process_rx_dma(transferred);
    }
    else if (huart->Instance->ISR & USART_ISR_IDLE) { // Check if IDLE and happen to be TC or HT
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

// Robustly restart the RX path using the same Rx-to-IDLE DMA mode as init
static void rs422_restart_rx(UART_HandleTypeDef *huart)
{
    // Stop any ongoing RX and associated DMA to reset HAL state machine
    HAL_UART_AbortReceive(huart);
    HAL_UART_DMAStop(huart);

    // Clear UART error flags
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_NEF |
                                 UART_CLEAR_PEF  | UART_CLEAR_FEF);

    // Reset software buffer indices to avoid misaligned parsing
    rx_buffer.read_pos = 0;
    rx_buffer.write_pos = 0;

    // Re-arm Rx-to-IDLE reception (use same mode as rs422_init)
    HAL_StatusTypeDef st = HAL_UARTEx_ReceiveToIdle_DMA(
        huart,
        (uint8_t*)rx_buffer.buffer, // cast away volatile for HAL API
        RS422_RX_BUFFER_SIZE
    );

    // We rely on IDLE events, not DMA HT/TC
    if (huart->hdmarx) {
        __HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT);
        __HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_TC);
    }

    if (st != HAL_OK) {
        dbg_printf("RS422: RX restart failed (status %d)\r\n", st);
    } else {
        dbg_printf("RS422: RX restarted after error\r\n");
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    // TODO: Test if this actually correctly restart the RS422 link. Also could abort on RS422 error.
    if (huart->Instance == USART1) {
        dbg_printf("RS422 UART Error: 0x%08lX\r\n", huart->ErrorCode);

        // Fully reset and re-arm the RX path in the same mode as init
        rs422_restart_rx(huart);
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

    // Clear all RS422 errors before starting
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_NEF | 
                         UART_CLEAR_PEF | UART_CLEAR_FEF);

    // Start continuous DMA reception
    __HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT);
    __HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_TC);
    HAL_StatusTypeDef status = HAL_UARTEx_ReceiveToIdle_DMA(huart, (uint8_t*)rx_buffer.buffer, RS422_RX_BUFFER_SIZE);

    // Debug output
    if (status == HAL_OK) {
        dbg_printf("RS422 init: DMA reception started successfully\r\n");
    } else {
        dbg_printf("RS422 init: DMA reception failed with status %d\r\n", status);
    }

    return (status == HAL_OK);
}

HAL_StatusTypeDef rs422_send(uint8_t *data, uint8_t size, RS422_FrameType_t frame_type)
{
    //HAL_GPIO_TogglePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin); // Toggle status LED for debugging
    if (size > RS422_TX_MESSAGE_SIZE - 3) {
        dbg_printf("RS422 TX: data size %d too large\r\n", size);
        return HAL_ERROR; // Data too large for packet
    }
    
    // Check if there's space in the buffer
    if (rs422_get_tx_buffer_space() == 0) {
        dbg_printf("RS422 TX: buffer full, cannot send frame\r\n");
        return HAL_BUSY; // Buffer full
    }

    uint8_t dlc = len_to_dlc(size); // Convert length to DLC format
    size = dlc_to_len(dlc); // Get actual size based on DLC

    // Compute Header
    uint8_t header = frame_type << 4; // Shift frame type to the upper nibble
    header |= dlc & 0x0F; // Set the lower nibble to the DLC
    
    // Copy data to the next packet in the buffer
    memcpy(&tx_buffer.buffer[tx_buffer.tail].data[0], &header, 1); // Set header byte
    memcpy(&tx_buffer.buffer[tx_buffer.tail].data[1], data, size); // Copy data bytes

    // Compute CRC-16 using the HAL of header and data
    uint16_t crc = crc16_compute(tx_buffer.buffer[tx_buffer.tail].data, size + 1); // Include header in CRC calculation

    // Append CRC to the packet
    memcpy(&tx_buffer.buffer[tx_buffer.tail].data[size + 1], &crc, 2); // Copy CRC after data

    tx_buffer.buffer[tx_buffer.tail].size = size + 3; // Store total size (header + data + CRC)
    tx_buffer.tail = (tx_buffer.tail + 1) % RS422_TX_BUFFER_SIZE;

    //HAL_GPIO_TogglePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin); // Toggle status LED for debugging
    
    // Start DMA transfer if not already in progress
    if (!tx_buffer.is_busy) {
        return rs422_process_tx_queue();
    }

    return HAL_OK;
}

uint16_t rs422_get_tx_buffer_space(void)
{
    return RS422_TX_BUFFER_SIZE - ((tx_buffer.tail - tx_buffer.head + RS422_TX_BUFFER_SIZE) % RS422_TX_BUFFER_SIZE);
}

HAL_StatusTypeDef rs422_process_tx_queue(void)
{
    if (tx_buffer.is_busy) {
        return HAL_BUSY;
    }
    
    // Check if there are packets to send
    if (tx_buffer.head == tx_buffer.tail) {
        return HAL_OK;
    }
    
    // Start DMA transfer for the next packet with actual data size
    tx_buffer.is_busy = true;
    return HAL_UART_Transmit_DMA(rs422_uart_handle, 
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

uint16_t rs422_read(RS422_RxFrame_t *frame)
{
    static uint8_t message_buffer[RS422_TX_MESSAGE_SIZE]; // Static buffer for reading data
    uint16_t available = rs422_get_rx_available();
    if (available == 0) {
        return 0; // No data available
    }

    uint8_t header = rx_buffer.buffer[rx_buffer.read_pos]; // Read header byte
    uint8_t data_length = dlc_to_len(header & 0x0F); // Actual data length
    if (data_length > available - 3) {
        return 0; // Not enough data available for a complete packet
    }
    frame->size = data_length;
    frame->frame_type = (header >> 4) & 0x0F; // Extract frame type from header

    uint16_t bytes_read = 0;
    uint16_t to_read = 1 + data_length + 2; // header + data + CRC

    // Read data from circular buffer
    while (bytes_read < to_read) {
        message_buffer[bytes_read] = rx_buffer.buffer[rx_buffer.read_pos];
        rx_buffer.read_pos = (rx_buffer.read_pos + 1) % RS422_RX_BUFFER_SIZE;
        bytes_read++;
    }

    // Check CRC to validate the packet
    uint16_t crc_received = crc16_compute(message_buffer, data_length+1); // Compute CRC on received data
    uint16_t crc_in_packet = (uint16_t)message_buffer[1 + data_length] | ((uint16_t)message_buffer[2 + data_length] << 8);
    if (crc_received != crc_in_packet) {
        dbg_printf("RS422 CRC mismatch: computed 0x%04X, received 0x%04X\r\n", crc_received, crc_in_packet);
        rx_buffer.read_pos = rx_buffer.write_pos; // Discard packet and hope write position isn't part way through a new packet
        return 0; // CRC mismatch, invalid packet
    }

    // Copy validated data to the frame
    memcpy(frame->data, &message_buffer[1], data_length);
    return bytes_read;
}

// Needs a uint8_t in the form [Servo A Pos, Servo B Pos, Servo C Pos, Servo D Pos, Servos Armed, Any Servos Error, 0 (Solenoid Position), 0 (Pyro Armed)]
bool rs422_send_valve_position(uint8_t valve_pos)
{
    valve_pos = (valve_pos | spicy_get_solenoid() << 1 | spicy_get_arm()); // Add solenoid and pyro armed states
    return (rs422_send(&valve_pos, 1, RS422_FRAME_VALVE_UPDATE) == HAL_OK);
}

bool rs422_send_data(const uint8_t *data, uint8_t size, RS422_FrameType_t frame_type)
{
    return (rs422_send((uint8_t*)data, size, frame_type) == HAL_OK);
}

bool rs422_send_countdown(int8_t countdown)
{
    return (rs422_send((uint8_t*)&countdown, 1, RS422_FRAME_COUNTDOWN) == HAL_OK);
}

bool rs422_send_heartbeat(void)
{
    uint8_t heartbeat_msb = 0;
    uint8_t fsm_state = fsm_get_state();
    if (fsm_state == STATE_SEQUENCER) {
        heartbeat_msb = fsm_state << 4 | (uint8_t)sequencer_get_state();
    } else if (fsm_state == STATE_ERROR) {
        heartbeat_msb = fsm_state << 4 | fsm_get_error_code(); // Send last error code in lower nibble
    } else {
        heartbeat_msb = fsm_state << 4; // Other states have no sub-state
    }
    servo_status_u servo_status;
    servo_status_get(&servo_status);
    uint8_t heartbeat_lsb = (servo_status.status.mainState & 0x03) << 6;
    uint8_t heartbeat [2] = {heartbeat_msb, heartbeat_lsb};
    bool result = (rs422_send(heartbeat, 2, RS422_FRAME_HEARTBEAT) == HAL_OK);
    return result;
}

bool rs422_send_battery_state(uint8_t percent_2s, uint8_t percent_6s)
{
    uint8_t battery_state [2] = {percent_2s, percent_6s};
    return (rs422_send(battery_state, 2, RS422_BATTERY_VOLTAGE_FRAME) == HAL_OK);
}

bool rs422_send_string_message(const char *str, uint8_t length)
{
    if (length > RS422_TX_MESSAGE_SIZE - 3) {
        return false; // String too long for packet
    }
    return (rs422_send((uint8_t*)str, length, RS422_STRING_MESSAGE) == HAL_OK);
}