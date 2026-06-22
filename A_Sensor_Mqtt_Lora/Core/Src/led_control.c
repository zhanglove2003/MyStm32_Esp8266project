#include "led_control.h"

static uint16_t led_pin(uint8_t index)
{
  switch (index)
  {
    case 1: return GPIO_PIN_13;
    case 2: return GPIO_PIN_14;
    case 3: return GPIO_PIN_15;
    default: return 0;
  }
}

void LedControl_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void LedControl_Set(uint8_t index, uint8_t state)
{
  uint16_t pin = led_pin(index);

  if (pin == 0U)
  {
    return;
  }

  HAL_GPIO_WritePin(GPIOB, pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void LedControl_AllOff(void)
{
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);
}
