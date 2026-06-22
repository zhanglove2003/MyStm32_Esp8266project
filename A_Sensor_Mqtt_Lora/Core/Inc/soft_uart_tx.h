#ifndef __SOFT_UART_TX_H__
#define __SOFT_UART_TX_H__

#include "main.h"

void SoftUartTx_Init(void);
void SoftUartTx_SendString(const char *text);

#endif /* __SOFT_UART_TX_H__ */
