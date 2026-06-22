#ifndef __ENV_FRAME_H__
#define __ENV_FRAME_H__

#include <stdint.h>

typedef struct {
  int16_t temperature_tenths;
  uint16_t humidity_tenths;
  uint16_t co2_ppm;
} EnvFrame;

void EnvFrameParser_Init(void);
uint8_t EnvFrameParser_ProcessByte(uint8_t value, EnvFrame *frame);

#endif /* __ENV_FRAME_H__ */
