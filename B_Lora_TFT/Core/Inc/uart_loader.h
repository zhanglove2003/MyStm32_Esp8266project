#ifndef __UART_LOADER_H
#define __UART_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include <stdint.h>

uint8_t UART_LOADER_Run(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* __UART_LOADER_H */
