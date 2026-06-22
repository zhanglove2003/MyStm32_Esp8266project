#ifndef __LED_CONTROL_H__
#define __LED_CONTROL_H__

#include "main.h"
#include <stdint.h>

void LedControl_Init(void);
void LedControl_Set(uint8_t index, uint8_t state);
void LedControl_AllOff(void);

#endif /* __LED_CONTROL_H__ */
