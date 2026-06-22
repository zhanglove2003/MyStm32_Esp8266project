#include "main.h"
#include "soft_uart_rx.h"
#include "stm32f1xx_it.h"

void NMI_Handler(void)
{
  NVIC_SystemReset();
}

void HardFault_Handler(void)
{
  NVIC_SystemReset();
}

void MemManage_Handler(void)
{
  NVIC_SystemReset();
}

void BusFault_Handler(void)
{
  NVIC_SystemReset();
}

void UsageFault_Handler(void)
{
  NVIC_SystemReset();
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
  HAL_IncTick();
}

void EXTI15_10_IRQHandler(void)
{
  if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_11) != RESET)
  {
    SoftUartRx_EXTI_Callback(GPIO_PIN_11);
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);
  }
}
