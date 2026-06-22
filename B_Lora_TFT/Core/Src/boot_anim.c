#include "boot_anim.h"
#include "tft.h"
#include "w25qxx.h"

#define BOOT_ANIM_HEADER_SIZE 32U
#define BOOT_ANIM_CHUNK_SIZE 1024U
#define BOOT_ANIM_MAGIC "BANI"

static uint16_t read_le16(const uint8_t *data)
{
  return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t read_le32(const uint8_t *data)
{
  return (uint32_t)data[0] |
         ((uint32_t)data[1] << 8) |
         ((uint32_t)data[2] << 16) |
         ((uint32_t)data[3] << 24);
}

static uint16_t cached_image_count = 0;
static uint16_t cached_frame_delay_ms = 100;
static uint8_t is_header_valid = 0;
static uint8_t frame_buffer[BOOT_ANIM_CHUNK_SIZE];

static void parse_header_if_needed(void)
{
  uint8_t header[BOOT_ANIM_HEADER_SIZE];

  if (is_header_valid)
  {
    return;
  }

  if (!W25QXX_ReadData(BOOT_ANIM_ADDRESS, header, sizeof(header)))
  {
    return;
  }

  if ((header[0] != BOOT_ANIM_MAGIC[0]) ||
      (header[1] != BOOT_ANIM_MAGIC[1]) ||
      (header[2] != BOOT_ANIM_MAGIC[2]) ||
      (header[3] != BOOT_ANIM_MAGIC[3]))
  {
    return;
  }

  uint16_t width = read_le16(&header[4]);
  uint16_t height = read_le16(&header[6]);
  uint32_t frame_bytes = read_le32(&header[12]);

  if ((width == BOOT_ANIM_WIDTH) &&
      (height == BOOT_ANIM_HEIGHT) &&
      (frame_bytes == BOOT_ANIM_FRAME_BYTES))
  {
    cached_image_count = read_le16(&header[8]);
    cached_frame_delay_ms = read_le16(&header[10]);
    if (cached_frame_delay_ms == 0U)
    {
      cached_frame_delay_ms = 100U;
    }
    is_header_valid = 1;
  }
}

uint16_t BOOT_ANIM_GetImageCount(void)
{
  parse_header_if_needed();
  return cached_image_count;
}

uint16_t BOOT_ANIM_GetFrameDelay(void)
{
  parse_header_if_needed();
  return cached_frame_delay_ms;
}

uint8_t BOOT_ANIM_ShowImage(uint16_t index)
{
  parse_header_if_needed();
  if (!is_header_valid || index >= cached_image_count)
  {
    return 0;
  }

  uint32_t data_address = BOOT_ANIM_ADDRESS + BOOT_ANIM_HEADER_SIZE + ((uint32_t)index * BOOT_ANIM_FRAME_BYTES);
  uint32_t remaining = BOOT_ANIM_FRAME_BYTES;

  TFT_BeginFrameWrite();
  while (remaining > 0U)
  {
    uint16_t chunk = (remaining > BOOT_ANIM_CHUNK_SIZE) ? BOOT_ANIM_CHUNK_SIZE : (uint16_t)remaining;
    if (!W25QXX_ReadData(data_address, frame_buffer, chunk))
    {
      TFT_EndFrameWrite();
      return 0;
    }
    TFT_WriteFrameData(frame_buffer, chunk);
    data_address += chunk;
    remaining -= chunk;
  }
  TFT_EndFrameWrite();

  return 1;
}

uint8_t BOOT_ANIM_Play(void)
{
  uint8_t header[BOOT_ANIM_HEADER_SIZE];
  uint16_t width;
  uint16_t height;
  uint16_t frames;
  uint16_t delay_ms;
  uint32_t frame_bytes;
  uint32_t data_address;
  uint16_t frame_index;

  if (!W25QXX_ReadData(BOOT_ANIM_ADDRESS, header, sizeof(header)))
  {
    return 0;
  }

  if ((header[0] != BOOT_ANIM_MAGIC[0]) ||
      (header[1] != BOOT_ANIM_MAGIC[1]) ||
      (header[2] != BOOT_ANIM_MAGIC[2]) ||
      (header[3] != BOOT_ANIM_MAGIC[3]))
  {
    return 0;
  }

  width = read_le16(&header[4]);
  height = read_le16(&header[6]);
  frames = read_le16(&header[8]);
  delay_ms = read_le16(&header[10]);
  frame_bytes = read_le32(&header[12]);

  if ((width != BOOT_ANIM_WIDTH) ||
      (height != BOOT_ANIM_HEIGHT) ||
      (frames == 0U) ||
      (frame_bytes != BOOT_ANIM_FRAME_BYTES))
  {
    return 0;
  }

  data_address = BOOT_ANIM_ADDRESS + BOOT_ANIM_HEADER_SIZE;
  for (frame_index = 0; frame_index < frames; frame_index++)
  {
    uint32_t remaining = frame_bytes;
    uint32_t read_address = data_address + ((uint32_t)frame_index * frame_bytes);

    TFT_BeginFrameWrite();
    while (remaining > 0U)
    {
      uint16_t chunk = (remaining > BOOT_ANIM_CHUNK_SIZE) ? BOOT_ANIM_CHUNK_SIZE : (uint16_t)remaining;
      if (!W25QXX_ReadData(read_address, frame_buffer, chunk))
      {
        TFT_EndFrameWrite();
        return 0;
      }
      TFT_WriteFrameData(frame_buffer, chunk);
      read_address += chunk;
      remaining -= chunk;
    }
    TFT_EndFrameWrite();
    HAL_Delay(delay_ms);
  }

  return 1;
}
