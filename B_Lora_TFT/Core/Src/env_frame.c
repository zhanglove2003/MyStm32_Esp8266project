#include "env_frame.h"

#include <string.h>

#define FRAME_BUFFER_SIZE 48U

static char frame_buffer[FRAME_BUFFER_SIZE];
static uint8_t frame_index;
static uint8_t frame_active;

static uint8_t parse_unsigned(const char **cursor, uint16_t *value)
{
  uint32_t result = 0;
  uint8_t digits = 0;

  while ((**cursor >= '0') && (**cursor <= '9'))
  {
    result = (result * 10U) + (uint32_t)(**cursor - '0');
    (*cursor)++;
    digits++;
  }

  if ((digits == 0U) || (result > 65535U))
  {
    return 0;
  }

  *value = (uint16_t)result;
  return 1;
}

static uint8_t parse_tenths(const char **cursor, int16_t *value)
{
  uint16_t whole;
  uint16_t decimal = 0;
  int16_t sign = 1;

  if (**cursor == '-')
  {
    sign = -1;
    (*cursor)++;
  }

  if (!parse_unsigned(cursor, &whole))
  {
    return 0;
  }

  if (**cursor == '.')
  {
    (*cursor)++;
    if ((**cursor >= '0') && (**cursor <= '9'))
    {
      decimal = (uint16_t)(**cursor - '0');
      (*cursor)++;
    }
  }

  *value = (int16_t)(sign * (int16_t)((whole * 10U) + decimal));
  return 1;
}

static uint8_t parse_frame(const char *text, EnvFrame *frame)
{
  int16_t temperature;
  int16_t humidity_signed;
  uint16_t co2;

  if ((text[0] != 'T') || (text[1] != '='))
  {
    return 0;
  }
  text += 2;
  if (!parse_tenths(&text, &temperature) || (*text != ','))
  {
    return 0;
  }
  text++;

  if ((text[0] != 'H') || (text[1] != '='))
  {
    return 0;
  }
  text += 2;
  if (!parse_tenths(&text, &humidity_signed) || (*text != ',') || (humidity_signed < 0))
  {
    return 0;
  }
  text++;

  if ((text[0] != 'C') || (text[1] != '='))
  {
    return 0;
  }
  text += 2;
  if (!parse_unsigned(&text, &co2) || (*text != '\0'))
  {
    return 0;
  }

  frame->temperature_tenths = temperature;
  frame->humidity_tenths = (uint16_t)humidity_signed;
  frame->co2_ppm = co2;
  return 1;
}

void EnvFrameParser_Init(void)
{
  frame_index = 0;
  frame_active = 0;
  memset(frame_buffer, 0, sizeof(frame_buffer));
}

uint8_t EnvFrameParser_ProcessByte(uint8_t value, EnvFrame *frame)
{
  if (value == '$')
  {
    frame_index = 0;
    frame_active = 1;
    memset(frame_buffer, 0, sizeof(frame_buffer));
    return 0;
  }

  if (!frame_active)
  {
    return 0;
  }

  if (value == '#')
  {
    frame_buffer[frame_index] = '\0';
    frame_active = 0;

    if (frame != 0)
    {
      return parse_frame(frame_buffer, frame);
    }
    return 0;
  }

  if (frame_index < (FRAME_BUFFER_SIZE - 1U))
  {
    frame_buffer[frame_index++] = (char)value;
  }
  else
  {
    frame_active = 0;
    frame_index = 0;
  }

  return 0;
}
