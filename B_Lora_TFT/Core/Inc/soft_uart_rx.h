#ifndef __SOFT_UART_RX_H__
#define __SOFT_UART_RX_H__

#include "main.h"
#include <stdint.h>

void SoftUartRx_Init(void);
uint8_t SoftUartRx_ReadByte(uint8_t *value);
void SoftUartRx_EXTI_Callback(uint16_t GPIO_Pin);

#endif /* __SOFT_UART_RX_H__ */
