#ifndef __BOOT_ANIM_H
#define __BOOT_ANIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include <stdint.h>

#define BOOT_ANIM_ADDRESS 0x000000UL
#define BOOT_ANIM_WIDTH 160U
#define BOOT_ANIM_HEIGHT 80U
#define BOOT_ANIM_FRAME_BYTES (BOOT_ANIM_WIDTH * BOOT_ANIM_HEIGHT * 2U)

uint8_t BOOT_ANIM_Play(void);
uint16_t BOOT_ANIM_GetImageCount(void);
uint16_t BOOT_ANIM_GetFrameDelay(void);
uint8_t BOOT_ANIM_ShowImage(uint16_t index);

#ifdef __cplusplus
}
#endif

#endif /* __BOOT_ANIM_H */
