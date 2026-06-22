#ifndef __LED_COMMAND_H__
#define __LED_COMMAND_H__

#include <stdint.h>

typedef enum {
  LED_TARGET_A = 1,
  LED_TARGET_B = 2,
  LED_TARGET_ALL = 3
} LedTarget;

typedef struct {
  LedTarget target;
  uint8_t led;
  uint8_t state;
} LedCommand;

uint8_t LedCommand_Parse(const char *payload, LedCommand *command);
const char *LedCommand_TargetString(LedTarget target);

#endif /* __LED_COMMAND_H__ */
