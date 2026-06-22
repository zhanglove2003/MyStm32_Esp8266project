#ifndef __ENV_FONT_H
#define __ENV_FONT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include <stdint.h>

#define ENV_FONT_ADDRESS 0x180000UL
#define ENV_FONT_GLYPH_WIDTH 16U
#define ENV_FONT_GLYPH_HEIGHT 16U
#define ENV_FONT_GLYPH_BYTES 32U

uint8_t EnvFont_InitStorage(void);
uint8_t EnvFont_ReadGlyph(uint16_t codepoint, uint8_t glyph[ENV_FONT_GLYPH_BYTES]);
void EnvFont_DrawGlyph(uint16_t x, uint16_t y, uint16_t codepoint, uint16_t color, uint16_t bg);
void EnvFont_DrawText(uint16_t x, uint16_t y, const uint16_t *text, uint8_t count,
                      uint16_t color, uint16_t bg);

#ifdef __cplusplus
}
#endif

#endif /* __ENV_FONT_H */
