/* USER CODE BEGIN Header */
/*
 * STM32F103C8T6 A39C receiver + 0.96 inch 80x160 TFT display.
 */
/* USER CODE END Header */

#include "main.h"
#include "boot_anim.h"
#include "control_frame.h"
#include "env_font.h"
#include "env_frame.h"
#include "led_control.h"
#include "soft_uart_rx.h"
#include "tft.h"
#include "w25qxx.h"
#include <stdio.h>
#include <string.h>

SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;
IWDG_HandleTypeDef hiwdg;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_IWDG_Init(void);
static void Display_Loading(void);
static void Display_Home(uint16_t frame_index);
static void Display_EnvPageLayout(void);
static void Display_EnvValues(const EnvFrame *frame, uint8_t has_frame);
static void Format_Tenths(char *buffer, size_t size, int16_t tenths);

typedef enum
{
  APP_PAGE_HOME = 0,
  APP_PAGE_ENV
} AppPage;

static void Print_RuntimeDiagnostics(AppPage page, uint8_t font_ok);

typedef struct
{
  GPIO_TypeDef *port;
  uint16_t pin;
  GPIO_PinState last_raw;
  GPIO_PinState stable;
  uint32_t changed_tick;
  uint8_t wait_release;
} Button;

#define KEY_K1_PIN GPIO_PIN_8
#define KEY_K2_PIN GPIO_PIN_9
#define KEY_GPIO_PORT GPIOB
#define BUTTON_DEBOUNCE_MS 20U
#define HOME_ANIM_MIN_DELAY_MS 120U

static const uint16_t text_loading[] = {0x663EU, 0x793AU, 0x5C4FU, 0x521DU, 0x59CBU, 0x5316U, 0x4E2DU};
static const uint16_t text_env_title[] = {0x73AFU, 0x5883U, 0x53C2U, 0x6570U};
static const uint16_t text_temp_label[] = {0x6E29U, 0x5EA6U, 0xFF1AU};
static const uint16_t text_hum_label[] = {0x6E7FU, 0x5EA6U, 0xFF1AU};
static const uint16_t text_co2_label[] = {0x4E8CU, 0x6C27U, 0x5316U, 0x78B3U, 0x6D53U, 0x5EA6U, 0xFF1AU};
static const uint16_t text_celsius[] = {0x2103U};
static uint8_t g_w25_available = 0;
static uint8_t g_font_ok = 0;
static uint16_t g_image_count = 0;

static uint8_t Button_Update(Button *button);
static void Draw_LoadingSpinner(uint8_t frame);
static void Draw_EnvLabelLine(uint16_t y, const uint16_t *label, uint8_t label_count);
static void Draw_EnvValueOnlyLine(uint16_t y, uint8_t label_count, const char *value, uint8_t draw_celsius);

int main(void)
{
  EnvFrame frame = {0};
  ControlFrameParser control_parser;
  LedControlFrame led_frame;
  uint8_t byte;
  uint8_t w25_id[3];
  uint8_t font_ok;
  uint16_t image_count;
  char msg[80];
  uint32_t last_frame_tick = 0;
  uint32_t last_diag_tick = 0;
  uint32_t last_home_anim_tick = 0;
  uint16_t home_anim_delay_ms = 100;
  uint16_t home_frame_index = 0;
  uint8_t has_frame = 0;
  AppPage page = APP_PAGE_HOME;
  Button key1 = {KEY_GPIO_PORT, KEY_K1_PIN, GPIO_PIN_SET, GPIO_PIN_SET, 0, 0};
  Button key2 = {KEY_GPIO_PORT, KEY_K2_PIN, GPIO_PIN_SET, GPIO_PIN_SET, 0, 0};

  HAL_Init();
  SystemClock_Config();
  MX_IWDG_Init();

  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  LedControl_Init();
  SoftUartRx_Init();
  EnvFrameParser_Init();
  ControlFrameParser_Init(&control_parser);

  HAL_GPIO_WritePin(W25QXX_CS_GPIO_Port, W25QXX_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_SET);

  TFT_Init();

  W25QXX_ReadID(w25_id);
  g_w25_available = W25QXX_IsValidID(w25_id);
  snprintf(msg, sizeof(msg), "W25QXX ID=%02X %02X %02X\r\n", w25_id[0], w25_id[1], w25_id[2]);
  HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), 100);

  font_ok = g_w25_available ? EnvFont_InitStorage() : 0U;
  image_count = g_w25_available ? BOOT_ANIM_GetImageCount() : 0U;
  home_anim_delay_ms = g_w25_available ? BOOT_ANIM_GetFrameDelay() : 100U;
  if (home_anim_delay_ms == 0U)
  {
    home_anim_delay_ms = 100U;
  }
  if (home_anim_delay_ms < HOME_ANIM_MIN_DELAY_MS)
  {
    home_anim_delay_ms = HOME_ANIM_MIN_DELAY_MS;
  }
  g_font_ok = font_ok;
  g_image_count = image_count;
  snprintf(msg, sizeof(msg), "ENV FONT init=%u images=%u\r\n",
           (unsigned int)font_ok,
           (unsigned int)image_count);
  HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), 100);

  Display_Loading();
  Display_Home(home_frame_index);
  if (g_image_count > 0U)
  {
    home_frame_index = (uint16_t)((home_frame_index + 1U) % g_image_count);
  }
  last_home_anim_tick = HAL_GetTick();

  HAL_UART_Transmit(&huart1, (uint8_t *)"B A39C RX READY\r\n", 17, 100);

  while (1)
  {
    HAL_IWDG_Refresh(&hiwdg);
    uint8_t rx_budget = 32;
    while ((rx_budget-- > 0U) && SoftUartRx_ReadByte(&byte))
    {
      if (EnvFrameParser_ProcessByte(byte, &frame))
      {
        HAL_UART_Transmit(&huart1, (uint8_t *)"\r\nFRAME OK\r\n", 12, 100);
        has_frame = 1;
        last_frame_tick = HAL_GetTick();
        if (page == APP_PAGE_ENV)
        {
          Display_EnvValues(&frame, has_frame);
        }
      }

      if (ControlFrameParser_ProcessByte(&control_parser, byte, &led_frame))
      {
        LedControl_Set(led_frame.led, led_frame.state);
        HAL_UART_Transmit(&huart1, (uint8_t *)"\r\nLED CMD\r\n", 11, 100);
      }
    }

    if (Button_Update(&key2) && (page != APP_PAGE_ENV))
    {
      page = APP_PAGE_ENV;
      HAL_UART_Transmit(&huart1, (uint8_t *)"PAGE ENV\r\n", 10, 100);
      Display_EnvPageLayout();
      Display_EnvValues(&frame, has_frame);
    }

    if (Button_Update(&key1) && (page != APP_PAGE_HOME))
    {
      page = APP_PAGE_HOME;
      HAL_UART_Transmit(&huart1, (uint8_t *)"PAGE HOME\r\n", 11, 100);
      Display_Home(home_frame_index);
      if (g_image_count > 0U)
      {
        home_frame_index = (uint16_t)((home_frame_index + 1U) % g_image_count);
      }
      last_home_anim_tick = HAL_GetTick();
    }

    if ((page == APP_PAGE_HOME) &&
        (g_w25_available != 0U) &&
        (g_image_count > 0U) &&
        ((HAL_GetTick() - last_home_anim_tick) >= home_anim_delay_ms))
    {
      last_home_anim_tick = HAL_GetTick();
      if (BOOT_ANIM_ShowImage(home_frame_index))
      {
        home_frame_index = (uint16_t)((home_frame_index + 1U) % g_image_count);
      }
    }

    if ((HAL_GetTick() - last_diag_tick) >= 1000U)
    {
      last_diag_tick = HAL_GetTick();
      Print_RuntimeDiagnostics(page, g_font_ok);
    }

    (void)last_frame_tick;
  }
}

static void Format_Tenths(char *buffer, size_t size, int16_t tenths)
{
  int16_t whole = (int16_t)(tenths / 10);
  int16_t decimal = (int16_t)(tenths % 10);

  if (decimal < 0)
  {
    decimal = (int16_t)-decimal;
  }

  snprintf(buffer, size, "%d.%d", whole, decimal);
}

static uint8_t Button_Update(Button *button)
{
  GPIO_PinState raw = HAL_GPIO_ReadPin(button->port, button->pin);
  uint32_t now = HAL_GetTick();

  if (raw != button->last_raw)
  {
    button->last_raw = raw;
    button->changed_tick = now;
  }

  if ((raw != button->stable) && ((now - button->changed_tick) >= BUTTON_DEBOUNCE_MS))
  {
    button->stable = raw;
    if (button->stable == GPIO_PIN_RESET)
    {
      if (!button->wait_release)
      {
        button->wait_release = 1;
        return 1;
      }
    }
    else
    {
      button->wait_release = 0;
    }
  }

  return 0;
}

static void Draw_LoadingSpinner(uint8_t frame)
{
  static const uint8_t x_pos[8] = {78, 84, 88, 84, 78, 72, 68, 72};
  static const uint8_t y_pos[8] = {8, 10, 16, 22, 24, 22, 16, 10};

  TFT_FillRect(64, 4, 32, 28, TFT_BLACK);
  for (uint8_t i = 0; i < 8U; i++)
  {
    uint8_t active = (uint8_t)((frame + i) & 0x07U);
    uint16_t color = (active == 0U) ? TFT_WHITE : TFT_BLUE;
    TFT_FillRect(x_pos[i], y_pos[i], 3, 3, color);
  }
}

static void Display_Loading(void)
{
  uint32_t start;
  uint8_t frame = 0;

  TFT_FillScreen(TFT_BLACK);
  EnvFont_DrawText(24, 38, text_loading, (uint8_t)(sizeof(text_loading) / sizeof(text_loading[0])),
                   TFT_WHITE, TFT_BLACK);

  start = HAL_GetTick();
  while ((HAL_GetTick() - start) < 5000U)
  {
    HAL_IWDG_Refresh(&hiwdg);
    Draw_LoadingSpinner(frame++);
    HAL_Delay(150);
  }
}

static void Display_Home(uint16_t frame_index)
{
  if (g_w25_available && (g_image_count > 0U) && BOOT_ANIM_ShowImage((uint16_t)(frame_index % g_image_count)))
  {
    return;
  }

  TFT_FillScreen(TFT_WHITE);
  if (!g_w25_available)
  {
    TFT_DrawString(4, 4, "W25 ERR", TFT_RED, TFT_WHITE);
  }
}

static void Print_RuntimeDiagnostics(AppPage page, uint8_t font_ok)
{
  uint8_t id[3];
  uint16_t image_count;
  char line[96];
  GPIO_PinState k1 = HAL_GPIO_ReadPin(KEY_GPIO_PORT, KEY_K1_PIN);
  GPIO_PinState k2 = HAL_GPIO_ReadPin(KEY_GPIO_PORT, KEY_K2_PIN);

  W25QXX_ReadID(id);
  g_w25_available = W25QXX_IsValidID(id);
  image_count = g_w25_available ? BOOT_ANIM_GetImageCount() : 0U;
  g_image_count = image_count;

  snprintf(line, sizeof(line),
           "W25=%02X %02X %02X FONT=%u IMG=%u K1=%u K2=%u PAGE=%u\r\n",
           id[0], id[1], id[2],
           (unsigned int)(g_w25_available ? font_ok : 0U),
           (unsigned int)image_count,
           (unsigned int)(k1 == GPIO_PIN_SET),
           (unsigned int)(k2 == GPIO_PIN_SET),
           (unsigned int)page);
  HAL_UART_Transmit(&huart1, (uint8_t *)line, strlen(line), 100);
}

static void Draw_EnvLabelLine(uint16_t y, const uint16_t *label, uint8_t label_count)
{
  EnvFont_DrawText(8, y, label, label_count, TFT_WHITE, TFT_BLACK);
}

static void Draw_EnvValueOnlyLine(uint16_t y, uint8_t label_count, const char *value, uint8_t draw_celsius)
{
  uint16_t value_x = (uint16_t)(8U + ((uint16_t)label_count * ENV_FONT_GLYPH_WIDTH));

  TFT_FillRect(value_x, y, (uint16_t)(TFT_WIDTH - value_x), 17, TFT_BLACK);
  TFT_DrawString(value_x, (uint16_t)(y + 5U), value, TFT_YELLOW, TFT_BLACK);

  if (draw_celsius)
  {
    uint16_t celsius_x = (uint16_t)(value_x + ((uint16_t)strlen(value) * 6U));
    EnvFont_DrawText(celsius_x, y, text_celsius, 1, TFT_YELLOW, TFT_BLACK);
  }
}

static void Display_EnvPageLayout(void)
{
  TFT_FillScreen(TFT_BLACK);
  EnvFont_DrawText(48, 2, text_env_title, (uint8_t)(sizeof(text_env_title) / sizeof(text_env_title[0])),
                   TFT_CYAN, TFT_BLACK);

  Draw_EnvLabelLine(22, text_temp_label, (uint8_t)(sizeof(text_temp_label) / sizeof(text_temp_label[0])));
  Draw_EnvLabelLine(40, text_hum_label, (uint8_t)(sizeof(text_hum_label) / sizeof(text_hum_label[0])));
  Draw_EnvLabelLine(58, text_co2_label, (uint8_t)(sizeof(text_co2_label) / sizeof(text_co2_label[0])));
}

static void Display_EnvValues(const EnvFrame *frame, uint8_t has_frame)
{
  char value[16];

  if (has_frame && (frame != 0))
  {
    Format_Tenths(value, sizeof(value), frame->temperature_tenths);
    Draw_EnvValueOnlyLine(22, (uint8_t)(sizeof(text_temp_label) / sizeof(text_temp_label[0])), value, 1);

    Format_Tenths(value, sizeof(value), (int16_t)frame->humidity_tenths);
    strncat(value, "%", sizeof(value) - strlen(value) - 1U);
    Draw_EnvValueOnlyLine(40, (uint8_t)(sizeof(text_hum_label) / sizeof(text_hum_label[0])), value, 0);

    snprintf(value, sizeof(value), "%u.%u%%",
             (unsigned int)(frame->co2_ppm / 100U),
             (unsigned int)((frame->co2_ppm / 10U) % 10U));
    Draw_EnvValueOnlyLine(58, (uint8_t)(sizeof(text_co2_label) / sizeof(text_co2_label[0])), value, 0);
  }
  else
  {
    Draw_EnvValueOnlyLine(22, (uint8_t)(sizeof(text_temp_label) / sizeof(text_temp_label[0])), "--", 1);
    Draw_EnvValueOnlyLine(40, (uint8_t)(sizeof(text_hum_label) / sizeof(text_hum_label[0])), "--%", 0);
    Draw_EnvValueOnlyLine(58, (uint8_t)(sizeof(text_co2_label) / sizeof(text_co2_label[0])), "--%", 0);
  }
}

static void MX_IWDG_Init(void)
{
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
  hiwdg.Init.Reload = 0xFFF;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    NVIC_SystemReset();
  }
}

static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_SPI1_Init(void)
{
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, W25QXX_CS_Pin | TFT_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, TFT_DC_Pin | TFT_RES_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = W25QXX_CS_Pin | TFT_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = TFT_DC_Pin | TFT_RES_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = KEY_K1_PIN | KEY_K2_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEY_GPIO_PORT, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  NVIC_SystemReset();
}
