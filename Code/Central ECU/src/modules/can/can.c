#include "can.h"
#include "debug_io.h"
#include "filters.h"
#include "can_handlers.h"

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


