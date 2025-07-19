#include "can_handler.h"
#include "debug_io.h"

FDCAN_TxHeaderTypeDef TxHeader;
FDCAN_RxHeaderTypeDef RxHeader;

uint8_t TxData[12];
uint8_t RxData[12];
uint16_t test_counter = 0;

FDCAN_FilterTypeDef sFilterConfig[4] = {{
    .IdType = FDCAN_STANDARD_ID,
    .FilterIndex = 0,
    .FilterType = FDCAN_FILTER_MASK,
    .FilterConfig = FDCAN_FILTER_TO_RXFIFO0,
    .FilterID1 = 0b10001000000, // Example ID
    .FilterID2 = 0b10111110000  // Example mask
}, {
    .IdType = FDCAN_STANDARD_ID,
    .FilterIndex = 1,
    .FilterType = FDCAN_FILTER_MASK,
    .FilterConfig = FDCAN_FILTER_TO_RXFIFO1,
    .FilterID1 = 0b00001000000, // Example ID
    .FilterID2 = 0b10111110000  // Example mask
}, {
    .IdType = FDCAN_STANDARD_ID,
    .FilterIndex = 2,
    .FilterType = FDCAN_FILTER_MASK,
    .FilterConfig = FDCAN_FILTER_TO_RXFIFO0,
    .FilterID1 = 0b10000000000, // Example ID
    .FilterID2 = 0b10111111000  // Example mask
}, {
    .IdType = FDCAN_STANDARD_ID,
    .FilterIndex = 3,
    .FilterType = FDCAN_FILTER_MASK,
    .FilterConfig = FDCAN_FILTER_TO_RXFIFO1,
    .FilterID1 = 0b000000000000, // Example ID
    .FilterID2 = 0b101111110000  // Example mask
}};

void CAN_Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
  if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
  {
    /* Retreive Rx messages from RX FIFO0 */
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, RxData) != HAL_OK) {
        /* Reception Error */
        CAN_Error_Handler();
    }
    if (HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) {
        /* Notification Error */
        CAN_Error_Handler();
    }
  }
}

void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs)
{
  if((RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) != RESET)
  {
    test_counter++;
    dbg_printf("Received CAN message with ID: %08lX, Data: %02X %02X\r\n", RxHeader.Identifier, RxData[0], RxData[1]);
    /* Retreive Rx messages from RX FIFO1 */
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO1, &RxHeader, RxData) != HAL_OK) {
        /* Reception Error */
        CAN_Error_Handler();
    }
    if (HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0) != HAL_OK) {
        /* Notification Error */
        CAN_Error_Handler();
    }
  }
}

void can_init(void) {
    // Initialize CAN peripheral
    // This function should configure the CAN peripheral, set up filters, and start the CAN bus
    for (int i = 0; i < 4; i++) {
        if (HAL_FDCAN_ConfigFilter(&hfdcan2, &sFilterConfig[i]) != HAL_OK) {
            CAN_Error_Handler();
        }
    }
    if(HAL_FDCAN_Start(&hfdcan2)!= HAL_OK) {
        CAN_Error_Handler();
    }
    if (HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0) != HAL_OK) {
        /* Notification Error */
        CAN_Error_Handler();
    }

}

void can_test_send(void) {
    // Example function to send a test CAN message
    TxHeader.Identifier = 0b00001001000; // Example ID
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

    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader, TxData) != HAL_OK) {
        CAN_Error_Handler();
    }
}
