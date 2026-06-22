#ifndef __TFT_H
#define __TFT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include <stdint.h>

#define TFT_WIDTH 160
#define TFT_HEIGHT 80

/* Landscape view for common 0.96 inch 80x160 ST7735S modules. */
#define TFT_XSTART 1
#define TFT_YSTART 26

#define TFT_BLACK 0x0000
#define TFT_BLUE 0x001F
#define TFT_RED 0xF800
#define TFT_GREEN 0x07E0
#define TFT_CYAN 0x07FF
#define TFT_MAGENTA 0xF81F
#define TFT_YELLOW 0xFFE0
#define TFT_WHITE 0xFFFF

void TFT_Init(void);
void TFT_FillScreen(uint16_t color);
void TFT_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void TFT_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void TFT_DrawChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg);
void TFT_DrawString(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t bg);
void TFT_DrawHexByte(uint16_t x, uint16_t y, uint8_t value, uint16_t color, uint16_t bg);
void TFT_BeginFrameWrite(void);
void TFT_WriteFrameData(const uint8_t *data, uint16_t size);
void TFT_EndFrameWrite(void);

#ifdef __cplusplus
}
#endif

#endif /* __TFT_H */
