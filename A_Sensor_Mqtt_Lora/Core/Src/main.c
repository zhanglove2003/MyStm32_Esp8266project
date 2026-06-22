/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "control_frame.h"
#include "dht12.h"
#include "esp8266.h"
#include "led_command.h"
#include "led_control.h"
#include "mq135.h"
#include "onenet.h"
#include "soft_uart_tx.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void MX_IWDG_Init(void)
{
  IWDG->KR = 0x5555;
  IWDG->PR = 0x04;
  IWDG->RLR = 0xFFF;
  while (IWDG->SR != 0U)
  {
  }
  IWDG->KR = 0xAAAA;
  IWDG->KR = 0xCCCC;
}

static void Debug_Print(const char *text)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)text, strlen(text), 100);
}

static void Handle_LedCommand(const LedCommand *command)
{
  char msg[96];
  char frame[16];

  if (command == 0)
  {
    return;
  }

  sprintf(msg, "LED CMD target=%s led=%u state=%u\r\n",
          LedCommand_TargetString(command->target), command->led, command->state);
  Debug_Print(msg);

  if ((command->target == LED_TARGET_A) || (command->target == LED_TARGET_ALL))
  {
    LedControl_Set(command->led, command->state);
  }

  if ((command->target == LED_TARGET_B) || (command->target == LED_TARGET_ALL))
  {
    if (ControlFrame_BuildLed(command->led, command->state, frame, sizeof(frame)))
    {
      SoftUartTx_SendString(frame);
      Debug_Print(frame);
      Debug_Print("\r\n");
    }
  }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  MX_IWDG_Init();
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  LedControl_Init();
  SoftUartTx_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(1000);
  DHTC12_Status init_status = DHTC12_Init();
  char msg[220];
  if (init_status == DHTC12_OK)
  {
    sprintf(msg, "DHTC12 Init OK, cal=%s\r\n", DHTC12_UsingCalibration() ? "YES" : "NO");
  }
  else if (init_status == DHTC12_ERR_CALIB)
  {
    sprintf(msg, "DHTC12 Init OK, cal=NO, using linear humidity fallback\r\n");
  }
  else
  {
    sprintf(msg, "DHTC12 Init Failed: %s, hal_error=0x%08X\r\n",
            DHTC12_StatusString(init_status), (unsigned int)DHTC12_LastHalError());
  }
  HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);

  sprintf(msg, "MQ135 Calibrating R0 in fresh air, target=%.0fppm...\r\n", MQ135_FRESH_AIR_PPM);
  HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
  HAL_StatusTypeDef mq135_init_status = MQ135_Calibrate();
  if (mq135_init_status == HAL_OK)
  {
    sprintf(msg, "MQ135 R0 Calibrated: %.2fkOhm\r\n", MQ135_GetR0());
  }
  else
  {
    sprintf(msg, "MQ135 R0 Fallback: %.2fkOhm, hal_status=%d\r\n",
            MQ135_GetR0(), (int)mq135_init_status);
  }
  HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);

  uint8_t esp_ready = 0;
  uint8_t onenet_ready = 0;
  uint32_t last_connect_try = 0;
  uint32_t last_publish_tick = 0;
  ESP8266_Status esp_status = ESP8266_Init();
  if (esp_status == ESP8266_OK)
  {
    uint8_t net_status;
    esp_ready = 1;
    net_status = OneNet_Connect();
    if (net_status == ONENET_OK)
    {
      net_status = OneNet_SubscribeCommands();
    }
    if (net_status == ONENET_OK)
    {
      onenet_ready = 1;
      Debug_Print("OneNet connected and subscribed\r\n");
    }
    else
    {
      sprintf(msg, "OneNet connect/sub failed: %s, connack=0x%02X\r\n",
              OneNet_StatusString(net_status), OneNet_LastConnAckCode());
      Debug_Print(msg);
    }
  }
  else
  {
    sprintf(msg, "ESP8266 Init Failed: %s, resp=%s\r\n",
            ESP8266_StatusString(esp_status), ESP8266_LastResponse());
    Debug_Print(msg);
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  DHT12_Data sensor;
  MQ135_Data gas;
  float last_temperature = 0.0f;
  float last_humidity = 0.0f;
  uint16_t last_co2 = 0;
  LedCommand led_command;
  while (1)
  {
    IWDG->KR = 0xAAAA;
    if (onenet_ready)
    {
      uint8_t command_status = OneNet_PollCommand(&led_command);
      if (command_status == ONENET_OK)
      {
        Handle_LedCommand(&led_command);
      }
      else if (command_status == ONENET_ERR_COMMAND_PACKET)
      {
        Debug_Print("OneNet command packet ignored\r\n");
      }
    }

    DHTC12_Status ret = DHT12_Read(&sensor);

    if (ret == 0)
    {
      last_temperature = sensor.temperature;
      last_humidity = sensor.humidity;
      sprintf(msg, "Temp: %.1fC, Hum: %.1f%%, cal=%s\r\n",
              sensor.temperature, sensor.humidity, sensor.used_calibration ? "YES" : "NO");
    }
    else if (ret == DHTC12_ERR_CRC)
    {
      sprintf(msg, "DHTC12 %s, hal_error=0x%08X, raw=%02X %02X %02X %02X %02X %02X\r\n",
              DHTC12_StatusString(ret), (unsigned int)sensor.hal_error,
              sensor.raw[0], sensor.raw[1], sensor.raw[2],
              sensor.raw[3], sensor.raw[4], sensor.raw[5]);
    }
    else
    {
      sprintf(msg, "DHTC12 %s, hal_error=0x%08X\r\n",
              DHTC12_StatusString(ret), (unsigned int)sensor.hal_error);
    }

    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);

    HAL_StatusTypeDef gas_ret = MQ135_Read(&gas);
    if (gas_ret == HAL_OK)
    {
      last_co2 = (uint16_t)gas.co2_ppm;
      sprintf(msg, "MQ135: adc=%u, pin=%.2fV, ao=%.2fV, CO2_est=%.0fppm, co2_pct=%.3f%%, airq=%.1f%%, level=%s, r0=%s\r\n",
              gas.adc_raw, gas.pin_voltage, gas.ao_voltage,
              gas.co2_ppm, gas.co2_percent, gas.airq_percent,
              MQ135_LevelString(gas.level), gas.calibrated ? "CAL" : "FALLBACK");
    }
    else
    {
      sprintf(msg, "MQ135 ADC_ERR, hal_status=%d\r\n", (int)gas_ret);
    }
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);

    sprintf(msg, "$T=%.1f,H=%.1f,C=%u#", last_temperature, last_humidity, last_co2);
    SoftUartTx_SendString(msg);
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
    HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 100);

    if (onenet_ready && ((HAL_GetTick() - last_publish_tick) >= 10000U))
    {
      uint8_t send_status = OneNet_SendData(&sensor, &gas);
      last_publish_tick = HAL_GetTick();
      if (send_status != ONENET_OK)
      {
        sprintf(msg, "OneNet publish failed: %s\r\n", OneNet_StatusString(send_status));
        Debug_Print(msg);
        onenet_ready = 0;
        last_connect_try = HAL_GetTick();
      }
    }

    if (!onenet_ready && ((HAL_GetTick() - last_connect_try) > 10000U))
    {
      uint8_t net_status;
      last_connect_try = HAL_GetTick();

      if (!esp_ready)
      {
        Debug_Print("ESP8266 reconnecting WiFi...\r\n");
        esp_status = ESP8266_Init();
        if (esp_status == ESP8266_OK)
        {
          esp_ready = 1;
        }
        else
        {
          sprintf(msg, "ESP8266 reconnect failed: %s, resp=%s\r\n",
                  ESP8266_StatusString(esp_status), ESP8266_LastResponse());
          Debug_Print(msg);
        }
      }

      if (esp_ready)
      {
        Debug_Print("OneNet reconnecting...\r\n");
        net_status = OneNet_Connect();
        if (net_status == ONENET_OK)
        {
          net_status = OneNet_SubscribeCommands();
        }
        if (net_status == ONENET_OK)
        {
          onenet_ready = 1;
          Debug_Print("OneNet reconnected and subscribed\r\n");
        }
        else
        {
          if (net_status == ONENET_ERR_TCP)
          {
            esp_ready = 0;
          }
          sprintf(msg, "OneNet reconnect failed: %s, connack=0x%02X\r\n",
                  OneNet_StatusString(net_status), OneNet_LastConnAckCode());
          Debug_Print(msg);
        }
      }
    }

    HAL_Delay(2000);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  NVIC_SystemReset();
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
