#include "can.h"
#include "debug_io.h"


FDCAN_TxHeaderTypeDef TxHeader;
FDCAN_RxHeaderTypeDef RxHeader;

uint8_t TxData[64];
uint16_t test_counter = 0;

void CAN_Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

// RX FIFO0 is reserved for high priority messages
// Handle messages in ISR context
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
  if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
  {
    CAN_Frame_t frame;
    /* Retreive Rx messages from RX FIFO0 */
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, frame.data) != HAL_OK) {
        /* Reception Error */
        CAN_Error_Handler();
    }
    if (HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) {
        /* Notification Error */
        CAN_Error_Handler();
    }

    CAN_ID id = unpack_can_id(RxHeader.Identifier);
    frame.id = id;
    frame.length = RxHeader.DataLength;

    dbg_printf("Received High Priority CAN message with ID: %08lX\r\n", RxHeader.Identifier);

    if (id.priority == CAN_PRIORITY_HEARTBEAT) {
        handle_heatbeat((CAN_HeartbeatFrame*)frame.data, id, RxHeader.RxTimestamp);
        return; // No further processing needed for heartbeat messages
    }

    switch (id.frameType) {
        case CAN_TYPE_ERROR:
            handle_error_warning((CAN_ErrorWarningFrame*)frame.data, id);
            break;
        case CAN_TYPE_COMMAND:
            handle_command((CAN_CommandFrame*)frame.data, id);
            break;
        default:
            dbg_printf("Frame type can't be parsed with high priority: %d\r\n", id.frameType);
            enqueue_can_frame(&frame);
            break;
    }
  }
}

// RX FIFO1 is reserved for low priority messages
// Messages should be queued and processed in the main loop
void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs)
{
  if((RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) != RESET) {
    test_counter++;
    CAN_Frame_t frame;

    /* Retreive Rx messages from RX FIFO1 */
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO1, &RxHeader, frame.data) != HAL_OK) {
        /* Reception Error */
        CAN_Error_Handler();
    }
    if (HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0) != HAL_OK) {
        /* Notification Error */
        CAN_Error_Handler();
    }

    CAN_ID id = unpack_can_id(RxHeader.Identifier);
    frame.id = id;
    frame.length = RxHeader.DataLength;

    enqueue_can_frame(&frame);

    dbg_printf("Queued CAN message with ID: %08lX\r\n", RxHeader.Identifier);
    }
}

void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef *hfdcan)
{
    HAL_GPIO_TogglePin(LED_IND_ERROR_GPIO_Port, LED_IND_ERROR_Pin); // Toggle LED2 on error
    uint32_t errorCode = HAL_FDCAN_GetError(hfdcan);
    dbg_printf("FDCAN Error: %08lX\r\n", errorCode);
}

void can_init(void) {
    // Initialize CAN peripheral
    // This function should configure the CAN peripheral, set up filters, and start the CAN bus
    for (int i = 0; i < 4; i++) {
        if (HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig[i]) != HAL_OK) {
            CAN_Error_Handler();
        }
    }
    if(HAL_FDCAN_Start(&hfdcan1)!= HAL_OK) {
        CAN_Error_Handler();
    }
    if (HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_RX_FIFO1_NEW_MESSAGE | FDCAN_IT_ERROR_WARNING, 0) != HAL_OK) {
        /* Notification Error */
        CAN_Error_Handler();
    }

}

static bool can_send(CAN_ID id, uint8_t *data, uint8_t length) {
    // Prepare and send a CAN command message
    TxHeader.Identifier = pack_can_id(id);
    TxHeader.IdType = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType = FDCAN_DATA_FRAME;
    TxHeader.DataLength = length; // Set data length
    TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    TxHeader.BitRateSwitch = FDCAN_BRS_ON;
    TxHeader.FDFormat = FDCAN_FD_CAN;
    TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker = 0;

    dbg_printf("Sending CAN command with ID: %08lX, length: %d\r\n", TxHeader.Identifier, length);

    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, data) != HAL_OK) {
        return false; // Transmission Error
    }
    return true; // Transmission Successful
}

bool can_send_error_warning(CAN_NodeType nodeType, CAN_NodeAddr nodeAddr, CAN_ErrorAction action, uint8_t errorCode) {
    CAN_ID id = {
        .priority = CAN_PRIORITY_CRITICAL,
        .nodeType = nodeType, // Use the provided node type
        .nodeAddr = nodeAddr, // Use the provided node address
        .frameType = CAN_TYPE_ERROR // Error is a warning message
    };
    uint32_t tick = HAL_GetTick();
    CAN_ErrorWarningFrame frame = {
        .what = action<<6 | BOARD_ID, // Use the action and board ID
        .why = errorCode, // Use the provided error code
        .timestamp = {
            (uint8_t)((tick >> 16) & 0xFF),
            (uint8_t)((tick >> 8) & 0xFF),
            (uint8_t)(tick & 0xFF)
        }
    };
    // Send the error warning frame
    return can_send(id, (uint8_t*)&frame, sizeof(frame));
}

bool can_send_command(CAN_NodeType nodeType, CAN_NodeAddr nodeAddr, CAN_CMDType commandType, uint8_t command) {
    CAN_ID id = {
        .priority = CAN_PRIORITY_COMMAND,
        .nodeType = nodeType, // Use the provided node type
        .nodeAddr = nodeAddr, // Use the provided node address
        .frameType = CAN_TYPE_COMMAND
    };
    uint32_t tick = HAL_GetTick();
    CAN_CommandFrame frame = {
        .what = commandType<<3 | BOARD_ID, // Use the command type and board ID
        .options = command, // Use the provided command
        .timestamp = {
            (uint8_t)((tick >> 16) & 0xFF),
            (uint8_t)((tick >> 8) & 0xFF),
            (uint8_t)(tick & 0xFF)
        }
    };
    return can_send(id, (uint8_t*)&frame, sizeof(frame));
}

bool can_send_status(CAN_NodeType nodeType, CAN_NodeAddr nodeAddr, uint8_t status, uint8_t substatus) {
    CAN_ID id = {
        .priority = CAN_PRIORITY_DATA,
        .nodeType = nodeType, // Use the provided node type
        .nodeAddr = nodeAddr, // Use the provided node address
        .frameType = CAN_TYPE_STATUS // Status is a data message
    };
    uint32_t tick = HAL_GetTick();
    CAN_StatusFrame frame = {
        .what = BOARD_ID,
        .state = status,
        .substate = substatus,
        .timestamp = {
            (uint8_t)((tick >> 16) & 0xFF),
            (uint8_t)((tick >> 8) & 0xFF),
            (uint8_t)(tick & 0xFF)
        }
    };
    return can_send(id, (uint8_t*)&frame, sizeof(frame));
}

bool can_send_servo_position(CAN_NodeType nodeType, CAN_NodeAddr nodeAddr, uint8_t connected, uint8_t position[4]) {
    CAN_ID id = {
        .priority = CAN_PRIORITY_DATA,
        .nodeType = nodeType, // Use the provided node type
        .nodeAddr = nodeAddr, // Use the provided node address
        .frameType = CAN_TYPE_SERVO_POS // Servo position is a data message
    };
    uint32_t tick = HAL_GetTick();
    CAN_ServoPosFrame frame = {
        .what = (connected & 0x0F) << 3 | BOARD_ID,
        .pos = {0}, // Initialize position array
        .timestamp = {
            (uint8_t)((tick >> 16) & 0xFF),
            (uint8_t)((tick >> 8) & 0xFF),
            (uint8_t)(tick & 0xFF)
        }
    };
    // Copy the position data into the frame
    memcpy(frame.pos, position, sizeof(frame.pos));
    return can_send(id, (uint8_t*)&frame, sizeof(frame));
}

bool can_send_heartbeat(CAN_NodeAddr nodeAddr) {
    CAN_ID id = {
        .priority = CAN_PRIORITY_HEARTBEAT,
        .nodeType = CAN_NODE_TYPE_CENTRAL, // Assuming this is the central node
        .nodeAddr = nodeAddr, // Use the provided node address
        .frameType = CAN_TYPE_HEARTBEAT // Heartbeat is a status message
    };
    uint32_t tick = HAL_GetTick();
    CAN_HeartbeatFrame frame = {
        .what = BOARD_ID,
        .timestamp = {
            (uint8_t)((tick >> 16) & 0xFF),
            (uint8_t)((tick >> 8) & 0xFF),
            (uint8_t)(tick & 0xFF)
        }
    };
    return can_send(id, (uint8_t*)&frame, sizeof(frame));
}

bool can_send_data(uint8_t sensorID, uint8_t *data, uint8_t length) {
    CAN_ID id = {
        .priority = CAN_PRIORITY_DATA,
        .nodeType = CAN_NODE_TYPE_CENTRAL, // Use the provided node type
        .nodeAddr = CAN_NODE_ADDR_CENTRAL, // Use the provided node address
        .frameType = CAN_TYPE_ADC_DATA // Data is a data message
    };
    uint32_t tick = HAL_GetTick();
    CAN_ADCFrame frame = {
        .what = BOARD_ID,
        .timestamp = {
            (uint8_t)((tick >> 16) & 0xFF),
            (uint8_t)((tick >> 8) & 0xFF),
            (uint8_t)(tick & 0xFF)
        }
    };
    memcpy(frame.data, data, length < sizeof(frame.data) ? length : sizeof(frame.data));
    return can_send(id, (uint8_t*)&frame, sizeof(frame));
}
