#include "esp8266.h"
#include "usart.h"
#include "onenet_config.h"
#include <stdio.h>
#include <string.h>

static uint8_t esp_rx_byte;
static volatile uint16_t esp_rx_len;
static uint8_t esp_rx_buf[ESP8266_RX_BUF_SIZE];
static char esp_last_response[ESP8266_RX_BUF_SIZE];

static void debug_print(const char *text)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)text, strlen(text), 100);
}

void ESP8266_RxStart(void)
{
  HAL_UART_Receive_IT(&huart2, &esp_rx_byte, 1);
}

void ESP8266_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    if (esp_rx_len < (ESP8266_RX_BUF_SIZE - 1))
    {
      esp_rx_buf[esp_rx_len++] = esp_rx_byte;
      esp_rx_buf[esp_rx_len] = 0;
    }
    HAL_UART_Receive_IT(&huart2, &esp_rx_byte, 1);
  }
}

void ESP8266_Clear(void)
{
  __disable_irq();
  esp_rx_len = 0;
  memset(esp_rx_buf, 0, sizeof(esp_rx_buf));
  __enable_irq();
}

const char *ESP8266_LastResponse(void)
{
  uint16_t len;

  __disable_irq();
  len = esp_rx_len;
  if (len >= sizeof(esp_last_response))
    len = sizeof(esp_last_response) - 1;
  memcpy(esp_last_response, esp_rx_buf, len);
  esp_last_response[len] = 0;
  __enable_irq();

  return esp_last_response;
}

ESP8266_Status ESP8266_SendCmd(const char *cmd, const char *expect, uint32_t timeout_ms)
{
  uint32_t start;

  ESP8266_Clear();
  if (HAL_UART_Transmit(&huart2, (uint8_t *)cmd, strlen(cmd), timeout_ms) != HAL_OK)
    return ESP8266_ERR_SEND;

  start = HAL_GetTick();
  while ((HAL_GetTick() - start) < timeout_ms)
  {
    if (strstr((const char *)esp_rx_buf, expect) != NULL)
      return ESP8266_OK;
    if (strstr((const char *)esp_rx_buf, "ERROR") != NULL || strstr((const char *)esp_rx_buf, "FAIL") != NULL)
      return ESP8266_ERR_SEND;
    IWDG->KR = 0xAAAA;
    HAL_Delay(10);
  }

  return ESP8266_ERR_TIMEOUT;
}

ESP8266_Status ESP8266_SendData(const uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
  char cmd[32];
  uint32_t start;

  sprintf(cmd, "AT+CIPSEND=%u\r\n", len);
  if (ESP8266_SendCmd(cmd, ">", 3000) != ESP8266_OK)
    return ESP8266_ERR_SEND;

  ESP8266_Clear();
  if (HAL_UART_Transmit(&huart2, (uint8_t *)data, len, timeout_ms) != HAL_OK)
    return ESP8266_ERR_SEND;

  start = HAL_GetTick();
  while ((HAL_GetTick() - start) < timeout_ms)
  {
    if (strstr((const char *)esp_rx_buf, "SEND OK") != NULL)
      return ESP8266_OK;
    if (strstr((const char *)esp_rx_buf, "ERROR") != NULL || strstr((const char *)esp_rx_buf, "FAIL") != NULL)
      return ESP8266_ERR_SEND;
    IWDG->KR = 0xAAAA;
    HAL_Delay(10);
  }

  return ESP8266_ERR_TIMEOUT;
}

ESP8266_Status ESP8266_WaitIPD(uint8_t *out, uint16_t max_len, uint16_t *out_len, uint32_t timeout_ms)
{
  uint32_t start = HAL_GetTick();

  while ((HAL_GetTick() - start) < timeout_ms)
  {
    uint16_t len_snapshot;
    uint16_t i;

    __disable_irq();
    len_snapshot = esp_rx_len;
    __enable_irq();

    for (i = 0; i + 5 < len_snapshot; i++)
    {
      if (esp_rx_buf[i] == '+' && esp_rx_buf[i + 1] == 'I' && esp_rx_buf[i + 2] == 'P' && esp_rx_buf[i + 3] == 'D' && esp_rx_buf[i + 4] == ',')
      {
        uint16_t p = i + 5;
        uint16_t data_len = 0;

        while (p < len_snapshot && esp_rx_buf[p] >= '0' && esp_rx_buf[p] <= '9')
        {
          data_len = (uint16_t)(data_len * 10 + esp_rx_buf[p] - '0');
          p++;
        }

        if (p < len_snapshot && esp_rx_buf[p] == ':')
        {
          p++;
          if ((uint16_t)(len_snapshot - p) >= data_len)
          {
            if (data_len > max_len)
              data_len = max_len;
            memcpy(out, &esp_rx_buf[p], data_len);
            *out_len = data_len;
            ESP8266_Clear();
            return ESP8266_OK;
          }
        }
      }
    }
    IWDG->KR = 0xAAAA;
    HAL_Delay(10);
  }

  *out_len = 0;
  return ESP8266_ERR_IPD;
}

ESP8266_Status ESP8266_OpenTcp(const char *host, uint16_t port)
{
  char cmd[96];

  sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",%u\r\n", host, port);
  return ESP8266_SendCmd(cmd, "CONNECT", 8000);
}

ESP8266_Status ESP8266_CloseTcp(void)
{
  ESP8266_Status ret;

  ret = ESP8266_SendCmd("AT+CIPCLOSE\r\n", "OK", 1000);
  ESP8266_Clear();
  return ret;
}

ESP8266_Status ESP8266_Init(void)
{
  char msg[96];
  char cmd[128];
  ESP8266_Status ret;

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
  HAL_Delay(250);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
  HAL_Delay(1000);

  ESP8266_RxStart();

  debug_print("ESP8266: AT\r\n");
  ret = ESP8266_SendCmd("AT\r\n", "OK", 2000);
  if (ret != ESP8266_OK) return ret;

  debug_print("ESP8266: CWMODE=1\r\n");
  ret = ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK", 2000);
  if (ret != ESP8266_OK) return ret;

  debug_print("ESP8266: connecting WiFi\r\n");
  sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", ONENET_WIFI_SSID, ONENET_WIFI_PASSWORD);
  ret = ESP8266_SendCmd(cmd, "GOT IP", 20000);
  sprintf(msg, "ESP8266 WiFi: %s\r\n", ESP8266_StatusString(ret));
  debug_print(msg);

  return ret;
}

const char *ESP8266_StatusString(ESP8266_Status status)
{
  switch (status)
  {
    case ESP8266_OK: return "OK";
    case ESP8266_ERR_TIMEOUT: return "TIMEOUT";
    case ESP8266_ERR_SEND: return "SEND_ERROR";
    case ESP8266_ERR_IPD: return "IPD_TIMEOUT";
    default: return "UNKNOWN";
  }
}
