/* USER CODE BEGIN Header */
/*
 * STM32F103C8T6 TFT bring-up test.
 */
/* USER CODE END Header */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

extern IWDG_HandleTypeDef hiwdg;

void Error_Handler(void);

#define W25QXX_CS_Pin GPIO_PIN_3
#define W25QXX_CS_GPIO_Port GPIOA
#define TFT_CS_Pin GPIO_PIN_4
#define TFT_CS_GPIO_Port GPIOA
#define TFT_DC_Pin GPIO_PIN_0
#define TFT_DC_GPIO_Port GPIOB
#define TFT_RES_Pin GPIO_PIN_1
#define TFT_RES_GPIO_Port GPIOB

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
