#ifndef __W25QXX_H
#define __W25QXX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include <stdint.h>

void W25QXX_ReadID(uint8_t id[3]);
uint8_t W25QXX_IsValidID(const uint8_t id[3]);
uint32_t W25QXX_GetCapacityBytes(const uint8_t id[3]);
uint8_t W25QXX_ReadData(uint32_t address, uint8_t *data, uint16_t length);
uint8_t W25QXX_PageProgram(uint32_t address, const uint8_t *data, uint16_t length);
uint8_t W25QXX_SectorErase(uint32_t address);

#ifdef __cplusplus
}
#endif

#endif /* __W25QXX_H */
