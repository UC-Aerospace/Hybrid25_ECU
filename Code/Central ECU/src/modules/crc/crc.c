#include "crc.h"

void crc16_init(void) {
    // __HAL_RCC_CRC_CLK_ENABLE();

    hcrc.Instance = CRC;
    hcrc.Init.DefaultPolynomialUse    = DEFAULT_POLYNOMIAL_DISABLE;
    hcrc.Init.GeneratingPolynomial    = 0x1021; // CRC-16-CCITT polynomial
    hcrc.Init.CRCLength               = CRC_POLYLENGTH_16B;
    hcrc.Init.DefaultInitValueUse     = DEFAULT_INIT_VALUE_DISABLE;
    hcrc.Init.InitValue              = 0xFFFF; // Standard init for CRC-16
    hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
    hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
    hcrc.InputDataFormat             = CRC_INPUTDATA_FORMAT_BYTES;
}

uint16_t crc16_compute(uint8_t *data, uint32_t length) {
    return (uint16_t) HAL_CRC_Calculate(&hcrc, (uint32_t *)data, length);
}
