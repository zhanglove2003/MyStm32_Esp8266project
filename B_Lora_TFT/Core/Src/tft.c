#include "tft.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;

#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT  0x11
#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5
#define ST7735_COLMOD  0x3A
#define ST7735_MADCTL  0x36
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_INVON   0x21
#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1
#define ST7735_NORON   0x13
#define ST7735_DISPON  0x29

#define TFT_SPI_TIMEOUT 100

static void TFT_Select(void)
{
  HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_RESET);
}

static void TFT_Unselect(void)
{
  HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_SET);
}

static void TFT_CommandMode(void)
{
  HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_RESET);
}

static void TFT_DataMode(void)
{
  HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_SET);
}

static void TFT_WriteCommand(uint8_t command)
{
  TFT_CommandMode();
  HAL_SPI_Transmit(&hspi1, &command, 1, TFT_SPI_TIMEOUT);
}

static void TFT_WriteData(const uint8_t *data, uint16_t size)
{
  TFT_DataMode();
  HAL_SPI_Transmit(&hspi1, (uint8_t *)data, size, TFT_SPI_TIMEOUT);
}

static void TFT_WriteDataByte(uint8_t data)
{
  TFT_WriteData(&data, 1);
}

static void TFT_Reset(void)
{
  HAL_GPIO_WritePin(TFT_RES_GPIO_Port, TFT_RES_Pin, GPIO_PIN_SET);
  HAL_Delay(20);
  HAL_GPIO_WritePin(TFT_RES_GPIO_Port, TFT_RES_Pin, GPIO_PIN_RESET);
  HAL_Delay(100);
  HAL_GPIO_WritePin(TFT_RES_GPIO_Port, TFT_RES_Pin, GPIO_PIN_SET);
  HAL_Delay(120);
}

static void TFT_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  uint8_t data[4];

  x0 += TFT_XSTART;
  x1 += TFT_XSTART;
  y0 += TFT_YSTART;
  y1 += TFT_YSTART;

  TFT_WriteCommand(ST7735_CASET);
  data[0] = (uint8_t)(x0 >> 8);
  data[1] = (uint8_t)x0;
  data[2] = (uint8_t)(x1 >> 8);
  data[3] = (uint8_t)x1;
  TFT_WriteData(data, sizeof(data));

  TFT_WriteCommand(ST7735_RASET);
  data[0] = (uint8_t)(y0 >> 8);
  data[1] = (uint8_t)y0;
  data[2] = (uint8_t)(y1 >> 8);
  data[3] = (uint8_t)y1;
  TFT_WriteData(data, sizeof(data));

  TFT_WriteCommand(ST7735_RAMWR);
}

void TFT_Init(void)
{
  TFT_Unselect();
  TFT_Reset();
  TFT_Select();

  TFT_WriteCommand(0x01); // ST7735_SWRESET
  HAL_Delay(150);

  TFT_WriteCommand(0x11); // ST7735_SLPOUT
  HAL_Delay(150);

  TFT_WriteCommand(0x3A); // ST7735_COLMOD
  TFT_WriteDataByte(0x05);
  HAL_Delay(10);

  TFT_WriteCommand(0x36); // ST7735_MADCTL
  TFT_WriteDataByte(0x68);

  TFT_WriteCommand(0x21); // ST7735_INVON
  TFT_WriteCommand(0x13); // ST7735_NORON
  HAL_Delay(10);

  TFT_WriteCommand(0x29); // ST7735_DISPON
  HAL_Delay(120);

  TFT_Unselect();
  TFT_FillScreen(0x0000); // TFT_BLACK
}

void TFT_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
  uint8_t data[2];

  if ((x >= TFT_WIDTH) || (y >= TFT_HEIGHT))
  {
    return;
  }

  data[0] = (uint8_t)(color >> 8);
  data[1] = (uint8_t)color;

  TFT_Select();
  TFT_SetAddressWindow(x, y, x, y);
  TFT_WriteData(data, sizeof(data));
  TFT_Unselect();
}

void TFT_FillScreen(uint16_t color)
{
  TFT_FillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}

void TFT_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
  uint8_t data[2];
  uint32_t pixels;

  if ((x >= TFT_WIDTH) || (y >= TFT_HEIGHT) || (width == 0U) || (height == 0U))
  {
    return;
  }

  if ((x + width) > TFT_WIDTH)
  {
    width = (uint16_t)(TFT_WIDTH - x);
  }
  if ((y + height) > TFT_HEIGHT)
  {
    height = (uint16_t)(TFT_HEIGHT - y);
  }

  pixels = (uint32_t)width * (uint32_t)height;

  data[0] = (uint8_t)(color >> 8);
  data[1] = (uint8_t)color;

  TFT_Select();
  TFT_SetAddressWindow(x, y, (uint16_t)(x + width - 1U), (uint16_t)(y + height - 1U));
  TFT_DataMode();

  while (pixels-- > 0)
  {
    HAL_SPI_Transmit(&hspi1, data, sizeof(data), TFT_SPI_TIMEOUT);
  }

  TFT_Unselect();
}

void TFT_BeginFrameWrite(void)
{
  TFT_Select();
  TFT_SetAddressWindow(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
  TFT_DataMode();
}

void TFT_WriteFrameData(const uint8_t *data, uint16_t size)
{
  TFT_Select();
  TFT_DataMode();
  HAL_SPI_Transmit(&hspi1, (uint8_t *)data, size, TFT_SPI_TIMEOUT);
  TFT_Unselect();
}

void TFT_EndFrameWrite(void)
{
  TFT_Unselect();
}

static const uint8_t *TFT_GetGlyph(char ch)
{
  static const uint8_t blank[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
  static const uint8_t glyph_0[5] = {0x3E, 0x51, 0x49, 0x45, 0x3E};
  static const uint8_t glyph_1[5] = {0x00, 0x42, 0x7F, 0x40, 0x00};
  static const uint8_t glyph_2[5] = {0x42, 0x61, 0x51, 0x49, 0x46};
  static const uint8_t glyph_3[5] = {0x21, 0x41, 0x45, 0x4B, 0x31};
  static const uint8_t glyph_4[5] = {0x18, 0x14, 0x12, 0x7F, 0x10};
  static const uint8_t glyph_5[5] = {0x27, 0x45, 0x45, 0x45, 0x39};
  static const uint8_t glyph_6[5] = {0x3C, 0x4A, 0x49, 0x49, 0x30};
  static const uint8_t glyph_7[5] = {0x01, 0x71, 0x09, 0x05, 0x03};
  static const uint8_t glyph_8[5] = {0x36, 0x49, 0x49, 0x49, 0x36};
  static const uint8_t glyph_9[5] = {0x06, 0x49, 0x49, 0x29, 0x1E};
  static const uint8_t glyph_A[5] = {0x7E, 0x11, 0x11, 0x11, 0x7E};
  static const uint8_t glyph_C[5] = {0x3E, 0x41, 0x41, 0x41, 0x22};
  static const uint8_t glyph_D[5] = {0x7F, 0x41, 0x41, 0x22, 0x1C};
  static const uint8_t glyph_E[5] = {0x7F, 0x49, 0x49, 0x49, 0x41};
  static const uint8_t glyph_F[5] = {0x7F, 0x09, 0x09, 0x09, 0x01};
  static const uint8_t glyph_G[5] = {0x3E, 0x41, 0x49, 0x49, 0x7A};
  static const uint8_t glyph_H[5] = {0x7F, 0x08, 0x08, 0x08, 0x7F};
  static const uint8_t glyph_I[5] = {0x00, 0x41, 0x7F, 0x41, 0x00};
  static const uint8_t glyph_K[5] = {0x7F, 0x08, 0x14, 0x22, 0x41};
  static const uint8_t glyph_L[5] = {0x7F, 0x40, 0x40, 0x40, 0x40};
  static const uint8_t glyph_M[5] = {0x7F, 0x02, 0x0C, 0x02, 0x7F};
  static const uint8_t glyph_N[5] = {0x7F, 0x04, 0x08, 0x10, 0x7F};
  static const uint8_t glyph_O[5] = {0x3E, 0x41, 0x41, 0x41, 0x3E};
  static const uint8_t glyph_P[5] = {0x7F, 0x09, 0x09, 0x09, 0x06};
  static const uint8_t glyph_Q[5] = {0x3E, 0x41, 0x51, 0x21, 0x5E};
  static const uint8_t glyph_R[5] = {0x7F, 0x09, 0x19, 0x29, 0x46};
  static const uint8_t glyph_S[5] = {0x46, 0x49, 0x49, 0x49, 0x31};
  static const uint8_t glyph_T[5] = {0x01, 0x01, 0x7F, 0x01, 0x01};
  static const uint8_t glyph_U[5] = {0x3F, 0x40, 0x40, 0x40, 0x3F};
  static const uint8_t glyph_V[5] = {0x1F, 0x20, 0x40, 0x20, 0x1F};
  static const uint8_t glyph_W[5] = {0x7F, 0x20, 0x18, 0x20, 0x7F};
  static const uint8_t glyph_X[5] = {0x63, 0x14, 0x08, 0x14, 0x63};
  static const uint8_t glyph_colon[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
  static const uint8_t glyph_slash[5] = {0x20, 0x10, 0x08, 0x04, 0x02};
  static const uint8_t glyph_dot[5] = {0x00, 0x60, 0x60, 0x00, 0x00};
  static const uint8_t glyph_minus[5] = {0x08, 0x08, 0x08, 0x08, 0x08};
  static const uint8_t glyph_percent[5] = {0x63, 0x13, 0x08, 0x64, 0x63};
  static const uint8_t glyph_equal[5] = {0x14, 0x14, 0x14, 0x14, 0x14};

  switch (ch)
  {
    case '0': return glyph_0;
    case '1': return glyph_1;
    case '2': return glyph_2;
    case '3': return glyph_3;
    case '4': return glyph_4;
    case '5': return glyph_5;
    case '6': return glyph_6;
    case '7': return glyph_7;
    case '8': return glyph_8;
    case '9': return glyph_9;
    case 'A': return glyph_A;
    case 'C': return glyph_C;
    case 'D': return glyph_D;
    case 'E': return glyph_E;
    case 'F': return glyph_F;
    case 'G': return glyph_G;
    case 'H': return glyph_H;
    case 'I': return glyph_I;
    case 'K': return glyph_K;
    case 'L': return glyph_L;
    case 'M': return glyph_M;
    case 'N': return glyph_N;
    case 'O': return glyph_O;
    case 'P': return glyph_P;
    case 'Q': return glyph_Q;
    case 'R': return glyph_R;
    case 'S': return glyph_S;
    case 'T': return glyph_T;
    case 'U': return glyph_U;
    case 'V': return glyph_V;
    case 'W': return glyph_W;
    case 'X': return glyph_X;
    case ':': return glyph_colon;
    case '/': return glyph_slash;
    case '.': return glyph_dot;
    case '-': return glyph_minus;
    case '%': return glyph_percent;
    case '=': return glyph_equal;
    default: return blank;
  }
}

void TFT_DrawChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg)
{
  const uint8_t *glyph = TFT_GetGlyph(ch);
  uint16_t col;
  uint16_t row;

  for (col = 0; col < 5; col++)
  {
    for (row = 0; row < 7; row++)
    {
      uint16_t pixel_color = (glyph[col] & (1U << row)) ? color : bg;
      TFT_DrawPixel((uint16_t)(x + col), (uint16_t)(y + row), pixel_color);
    }
  }

  for (row = 0; row < 7; row++)
  {
    TFT_DrawPixel((uint16_t)(x + 5), (uint16_t)(y + row), bg);
  }
}

void TFT_DrawString(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t bg)
{
  while (*text != '\0')
  {
    TFT_DrawChar(x, y, *text, color, bg);
    x = (uint16_t)(x + 6);
    text++;
  }
}

void TFT_DrawHexByte(uint16_t x, uint16_t y, uint8_t value, uint16_t color, uint16_t bg)
{
  static const char hex[] = "0123456789ABCDEF";

  TFT_DrawChar(x, y, hex[(value >> 4) & 0x0F], color, bg);
  TFT_DrawChar((uint16_t)(x + 6), y, hex[value & 0x0F], color, bg);
}
