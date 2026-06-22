#include "soft_uart_tx.h"

#include <stddef.h>

#define SOFT_UART_TX_PORT GPIOA
#define SOFT_UART_TX_PIN  GPIO_PIN_12
#define A39C_AUX_PORT     GPIOB
#define A39C_AUX_PIN      GPIO_PIN_12
#define SOFT_UART_BAUD    9600U
#define AUX_WAIT_TIMEOUT  100U

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

static void soft_uart_write(uint8_t level)
{
  HAL_GPIO_WritePin(SOFT_UART_TX_PORT, SOFT_UART_TX_PIN,
                    level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void soft_uart_wait_aux(void)
{
  uint32_t start = HAL_GetTick();

  while (HAL_GPIO_ReadPin(A39C_AUX_PORT, A39C_AUX_PIN) == GPIO_PIN_RESET)
  {
    if ((HAL_GetTick() - start) >= AUX_WAIT_TIMEOUT)
    {
      break;
    }
  }
}

static void soft_uart_send_byte(uint8_t value)
{
  soft_uart_write(0);
  dwt_delay_us(bit_us);

  for (uint8_t bit = 0; bit < 8; bit++)
  {
    soft_uart_write((uint8_t)(value & 0x01U));
    dwt_delay_us(bit_us);
    value >>= 1;
  }

  soft_uart_write(1);
  dwt_delay_us(bit_us);
}

void SoftUartTx_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  cycles_per_us = HAL_RCC_GetHCLKFreq() / 1000000U;
  bit_us = (1000000U + (SOFT_UART_BAUD / 2U)) / SOFT_UART_BAUD;

  HAL_GPIO_WritePin(SOFT_UART_TX_PORT, SOFT_UART_TX_PIN, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = SOFT_UART_TX_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SOFT_UART_TX_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = A39C_AUX_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(A39C_AUX_PORT, &GPIO_InitStruct);
}

void SoftUartTx_SendString(const char *text)
{
  if (text == NULL)
  {
    return;
  }

  soft_uart_wait_aux();
  while (*text != '\0')
  {
    __disable_irq();
    soft_uart_send_byte((uint8_t)*text);
    __enable_irq();
    text++;
  }
}
