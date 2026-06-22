#ifndef __CONTROL_FRAME_H__
#define __CONTROL_FRAME_H__

#include <stdint.h>
#include <stddef.h>

typedef struct {
  uint8_t led;
  uint8_t state;
} LedControlFrame;

typedef struct {
  char buffer[24];
  uint8_t index;
  uint8_t active;
} ControlFrameParser;

void ControlFrameParser_Init(ControlFrameParser *parser);
uint8_t ControlFrameParser_ProcessByte(ControlFrameParser *parser, uint8_t value, LedControlFrame *frame);
uint8_t ControlFrame_BuildLed(uint8_t led, uint8_t state, char *out, size_t out_size);

#endif /* __CONTROL_FRAME_H__ */
