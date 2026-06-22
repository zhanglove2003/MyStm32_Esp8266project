#include "soft_uart_rx.h"

#define SOFT_UART_RX_PORT GPIOA
#define SOFT_UART_RX_PIN  GPIO_PIN_12
#define A39C_AUX_PORT     GPIOB
#define A39C_AUX_PIN      GPIO_PIN_12
#define SOFT_UART_BAUD    9600U
static uint32_t cycles_per_us;
static uint32_t bit_us;

static void dwt_delay_us(uint32_t us)
{
  uint32_t start = DWT->CYCCNT;
  uint32_t cycles = us * cycles_per_us;

  while ((DWT->CYCCNT - start) < cycles)
  {
  }
}

static uint8_t soft_uart_receive_byte(uint8_t *value)
{
  uint8_t data = 0;

  if ((value == 0) ||
      (HAL_GPIO_ReadPin(SOFT_UART_RX_PORT, SOFT_UART_RX_PIN) != GPIO_PIN_RESET))
  {
    return 0;
  }

  dwt_delay_us(bit_us + (bit_us / 2U));
  for (uint8_t bit = 0; bit < 8; bit++)
  {
    if (HAL_GPIO_ReadPin(SOFT_UART_RX_PORT, SOFT_UART_RX_PIN) == GPIO_PIN_SET)
    {
      data |= (uint8_t)(1U << bit);
    }
    dwt_delay_us(bit_us);
  }

  *value = data;
  return 1;
}

void SoftUartRx_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  cycles_per_us = HAL_RCC_GetHCLKFreq() / 1000000U;
  bit_us = (1000000U + (SOFT_UART_BAUD / 2U)) / SOFT_UART_BAUD;

  GPIO_InitStruct.Pin = SOFT_UART_RX_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SOFT_UART_RX_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = A39C_AUX_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(A39C_AUX_PORT, &GPIO_InitStruct);
}

uint8_t SoftUartRx_ReadByte(uint8_t *value)
{
  return soft_uart_receive_byte(value);
}

void SoftUartRx_EXTI_Callback(uint16_t GPIO_Pin)
{
  (void)GPIO_Pin;
}
