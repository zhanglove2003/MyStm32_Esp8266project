#ifndef __ESP8266_H__
#define __ESP8266_H__

#include "main.h"
#include <stdint.h>

#define ESP8266_RX_BUF_SIZE 1024

typedef enum {
  ESP8266_OK = 0,
  ESP8266_ERR_TIMEOUT = 1,
  ESP8266_ERR_SEND = 2,
  ESP8266_ERR_IPD = 3
} ESP8266_Status;

void ESP8266_RxStart(void);
void ESP8266_RxCpltCallback(UART_HandleTypeDef *huart);
void ESP8266_Clear(void);
ESP8266_Status ESP8266_Init(void);
ESP8266_Status ESP8266_OpenTcp(const char *host, uint16_t port);
ESP8266_Status ESP8266_CloseTcp(void);
ESP8266_Status ESP8266_SendCmd(const char *cmd, const char *expect, uint32_t timeout_ms);
ESP8266_Status ESP8266_SendData(const uint8_t *data, uint16_t len, uint32_t timeout_ms);
ESP8266_Status ESP8266_WaitIPD(uint8_t *out, uint16_t max_len, uint16_t *out_len, uint32_t timeout_ms);
const char *ESP8266_StatusString(ESP8266_Status status);
const char *ESP8266_LastResponse(void);

#endif
