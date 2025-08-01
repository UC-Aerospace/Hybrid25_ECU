#include "can.h"
#include "debug_io.h"
#include "frames.h"
#include "handlers.h"
#include "filters.h"

FDCAN_TxHeaderTypeDef TxHeader;
FDCAN_RxHeaderTypeDef RxHeader;

uint8_t TxData[12];
uint16_t test_counter = 0;

void CAN_Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

// RX FIFO0 is reserved for high priority messages
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
  if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
  {
    uint8_t rxData[64];  // support FD CAN
    /* Retreive Rx messages from RX FIFO0 */
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, rxData) != HAL_OK) {
        /* Reception Error */
        CAN_Error_Handler();
    }
    if (HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) {
        /* Notification Error */
        CAN_Error_Handler();
    }

    dbg_printf("Received CAN message with ID: %08lX, Data: %02X %02X\r\n", RxHeader.Identifier, rxData[0], rxData[1]);
  }
}

void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs)
{
  if((RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) != RESET)
  {
    test_counter++;
    uint8_t rxData[64];  // CAN FD max length is 64 bytes

    /* Retreive Rx messages from RX FIFO1 */
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO1, &RxHeader, rxData) != HAL_OK) {
        /* Reception Error */
        CAN_Error_Handler();
    }
    if (HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0) != HAL_OK) {
        /* Notification Error */
        CAN_Error_Handler();
    }

    dbg_printf("Received CAN message with ID: %08lX, Data: %02X %02X\r\n", RxHeader.Identifier, rxData[0], rxData[1]);

    CAN_ID id = unpack_can_id(RxHeader.Identifier);

    switch (id.msgType) {
        case CAN_TYPE_ERROR:
            handle_error_warning((CAN_ErrorWarningFrame*)rxData, id, RxHeader.DataLength);
            break;
        case CAN_TYPE_COMMAND:
            handle_command((CAN_CommandFrame*)rxData, id, RxHeader.DataLength);
            break;
        case CAN_TYPE_STATUS:
            handle_status((CAN_StatusFrame*)rxData, id, RxHeader.DataLength);
            break;
        case CAN_TYPE_SERVO_POS:
            handle_servo_pos((CAN_ServoFrame*)rxData, id, RxHeader.DataLength);
            break;
        case CAN_TYPE_ADC_DATA:
            handle_adc_data((CAN_ADCFrame*)rxData, id, RxHeader.DataLength);
            break;
    }
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

void can_test_send(void) {
    // Example function to send a test CAN message
    TxHeader.Identifier = 0b10001000001; // Example ID
    TxHeader.IdType = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType = FDCAN_DATA_FRAME;
    TxHeader.DataLength = FDCAN_DLC_BYTES_2;
    TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    TxHeader.BitRateSwitch = FDCAN_BRS_ON;
    TxHeader.FDFormat = FDCAN_FD_CAN;
    TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker = 0;

    TxData[0] = test_counter & 0xFF; // Example data
    TxData[1] = (test_counter >> 8) & 0xFF;

    dbg_printf("Sending CAN message with ID: %08lX, Data: %02X %02X\r\n", TxHeader.Identifier, TxData[0], TxData[1]);

    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, TxData) != HAL_OK) {
        CAN_Error_Handler();
    }
}
