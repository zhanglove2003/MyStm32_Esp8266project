/**
  ******************************************************************************
  * @file    stm32f1xx_hal_iwdg.c
  * @brief   IWDG HAL module driver (minimal implementation).
  ******************************************************************************
  */

#include "stm32f1xx_hal.h"

#ifdef HAL_IWDG_MODULE_ENABLED

HAL_StatusTypeDef HAL_IWDG_Init(IWDG_HandleTypeDef *hiwdg)
{
  if (hiwdg == NULL)
  {
    return HAL_ERROR;
  }

  WRITE_REG(hiwdg->Instance->KR, IWDG_KEY_ENABLE);
  WRITE_REG(hiwdg->Instance->KR, IWDG_KEY_WRITE_ACCESS_ENABLE);
  WRITE_REG(hiwdg->Instance->PR, hiwdg->Init.Prescaler);
  WRITE_REG(hiwdg->Instance->RLR, hiwdg->Init.Reload);
  WRITE_REG(hiwdg->Instance->KR, IWDG_KEY_RELOAD);

  return HAL_OK;
}

HAL_StatusTypeDef HAL_IWDG_Refresh(IWDG_HandleTypeDef *hiwdg)
{
  if (hiwdg == NULL)
  {
    return HAL_ERROR;
  }

  WRITE_REG(hiwdg->Instance->KR, IWDG_KEY_RELOAD);

  return HAL_OK;
}

#endif /* HAL_IWDG_MODULE_ENABLED */
