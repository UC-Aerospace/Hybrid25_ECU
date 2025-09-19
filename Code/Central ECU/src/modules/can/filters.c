#include "filters.h"
#include "config.h"

#ifdef BOARD_TYPE_CENTRAL

FDCAN_FilterTypeDef sFilterConfig[4] = {{
    .IdType = FDCAN_STANDARD_ID,
    .FilterIndex = 0,
    .FilterType = FDCAN_FILTER_MASK,
    .FilterConfig = FDCAN_FILTER_TO_RXFIFO1,
    .FilterID1 = 0b10001000000, // Example ID
    .FilterID2 = 0b10111110000  // Example mask
}, {
    .IdType = FDCAN_STANDARD_ID,
    .FilterIndex = 1,
    .FilterType = FDCAN_FILTER_MASK,
    .FilterConfig = FDCAN_FILTER_TO_RXFIFO0,
    .FilterID1 = 0b00001000000, // Example ID
    .FilterID2 = 0b10111110000  // Example mask
}, {
    .IdType = FDCAN_STANDARD_ID,
    .FilterIndex = 2,
    .FilterType = FDCAN_FILTER_MASK,
    .FilterConfig = FDCAN_FILTER_TO_RXFIFO1,
    .FilterID1 = 0b10000000000, // Example ID
    .FilterID2 = 0b10111111000  // Example mask
}, {
    .IdType = FDCAN_STANDARD_ID,
    .FilterIndex = 3,
    .FilterType = FDCAN_FILTER_MASK,
    .FilterConfig = FDCAN_FILTER_TO_RXFIFO0,
    .FilterID1 = 0b000000000000, // Example ID
    .FilterID2 = 0b101111110000  // Example mask
}};

#endif // BOARD_TYPE_CENTRAL

#ifdef BOARD_TYPE_SERVO

FDCAN_FilterTypeDef sFilterConfig[4] = {{
    .IdType = FDCAN_STANDARD_ID,
    .FilterIndex = 0,
    .FilterType = FDCAN_FILTER_MASK,
    .FilterConfig = FDCAN_FILTER_TO_RXFIFO1,
    .FilterID1 = 0b10010010000, // Example ID
    .FilterID2 = 0b10111101000  // Example mask
}, {
    .IdType = FDCAN_STANDARD_ID,
    .FilterIndex = 1,
    .FilterType = FDCAN_FILTER_MASK,
    .FilterConfig = FDCAN_FILTER_TO_RXFIFO0,
    .FilterID1 = 0b00010010000, // Example ID
    .FilterID2 = 0b10111101000  // Example mask
}, {
    .IdType = FDCAN_STANDARD_ID,
    .FilterIndex = 2,
    .FilterType = FDCAN_FILTER_MASK,
    .FilterConfig = FDCAN_FILTER_TO_RXFIFO1,
    .FilterID1 = 0b10000000000, // Example ID
    .FilterID2 = 0b10111111000  // Example mask
}, {
    .IdType = FDCAN_STANDARD_ID,
    .FilterIndex = 3,
    .FilterType = FDCAN_FILTER_MASK,
    .FilterConfig = FDCAN_FILTER_TO_RXFIFO0,
    .FilterID1 = 0b000000000000, // Example ID
    .FilterID2 = 0b101111110000  // Example mask
}};

#endif // BOARD_TYPE_SERVO

#ifdef BOARD_TYPE_ADC

FDCAN_FilterTypeDef sFilterConfig[4] = {{
    .IdType = FDCAN_STANDARD_ID,
    .FilterIndex = 0,
    .FilterType = FDCAN_FILTER_MASK,
    .FilterConfig = FDCAN_FILTER_TO_RXFIFO1,
    .FilterID1 = 0b10011000000, // Example ID
    .FilterID2 = 0b10111100000  // Example mask
}, {
    .IdType = FDCAN_STANDARD_ID,
    .FilterIndex = 1,
    .FilterType = FDCAN_FILTER_MASK,
    .FilterConfig = FDCAN_FILTER_TO_RXFIFO0,
    .FilterID1 = 0b00011000000, // Example ID
    .FilterID2 = 0b10111100000  // Example mask
}, {
    .IdType = FDCAN_STANDARD_ID,
    .FilterIndex = 2,
    .FilterType = FDCAN_FILTER_MASK,
    .FilterConfig = FDCAN_FILTER_TO_RXFIFO1,
    .FilterID1 = 0b10000000000, // Example ID
    .FilterID2 = 0b10111111000  // Example mask
}, {
    .IdType = FDCAN_STANDARD_ID,
    .FilterIndex = 3,
    .FilterType = FDCAN_FILTER_MASK,
    .FilterConfig = FDCAN_FILTER_TO_RXFIFO0,
    .FilterID1 = 0b000000000000, // Example ID
    .FilterID2 = 0b101111110000  // Example mask
}};

#endif // BOARD_TYPE_ADC