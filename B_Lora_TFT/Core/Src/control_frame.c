#include "control_frame.h"

#include <stdio.h>
#include <string.h>

static uint8_t parse_digit_value(const char *text, const char *prefix, uint8_t *value)
{
  size_t len = strlen(prefix);

  if (strncmp(text, prefix, len) != 0)
  {
    return 0;
  }

  text += len;
  if ((*text < '0') || (*text > '9'))
  {
    return 0;
  }

  *value = (uint8_t)(*text - '0');
  return 1;
}

static uint8_t parse_led_frame(const char *text, LedControlFrame *frame)
{
  uint8_t led;
  uint8_t state;

  if (!parse_digit_value(text, "L=", &led))
  {
    return 0;
  }

  text += 3;
  if (*text != ',')
  {
    return 0;
  }
  text++;

  if (!parse_digit_value(text, "S=", &state))
  {
    return 0;
  }
  text += 3;

  if ((*text != '\0') || (led < 1U) || (led > 3U) || (state > 1U))
  {
    return 0;
  }

  frame->led = led;
  frame->state = state;
  return 1;
}

void ControlFrameParser_Init(ControlFrameParser *parser)
{
  if (parser == 0)
  {
    return;
  }

  parser->index = 0;
  parser->active = 0;
  memset(parser->buffer, 0, sizeof(parser->buffer));
}

uint8_t ControlFrameParser_ProcessByte(ControlFrameParser *parser, uint8_t value, LedControlFrame *frame)
{
  if ((parser == 0) || (frame == 0))
  {
    return 0;
  }

  if (value == '$')
  {
    parser->index = 0;
    parser->active = 1;
    memset(parser->buffer, 0, sizeof(parser->buffer));
    return 0;
  }

  if (!parser->active)
  {
    return 0;
  }

  if (value == '#')
  {
    parser->buffer[parser->index] = '\0';
    parser->active = 0;
    return parse_led_frame(parser->buffer, frame);
  }

  if (parser->index < (sizeof(parser->buffer) - 1U))
  {
    parser->buffer[parser->index++] = (char)value;
  }
  else
  {
    ControlFrameParser_Init(parser);
  }

  return 0;
}

uint8_t ControlFrame_BuildLed(uint8_t led, uint8_t state, char *out, size_t out_size)
{
  int written;

  if ((out == 0) || (out_size == 0U) || (led < 1U) || (led > 3U) || (state > 1U))
  {
    return 0;
  }

  written = snprintf(out, out_size, "$L=%u,S=%u#", led, state);
  if ((written < 0) || ((size_t)written >= out_size))
  {
    return 0;
  }

  return 1;
}
