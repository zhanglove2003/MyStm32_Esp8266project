#include "led_command.h"

#include <string.h>

static const char *skip_ws(const char *text)
{
  while ((*text == ' ') || (*text == '\t') || (*text == '\r') || (*text == '\n'))
  {
    text++;
  }
  return text;
}

static const char *find_json_key(const char *payload, const char *key)
{
  char pattern[24];

  if ((payload == 0) || (key == 0) || (strlen(key) > 16U))
  {
    return 0;
  }

  strcpy(pattern, "\"");
  strcat(pattern, key);
  strcat(pattern, "\"");

  return strstr(payload, pattern);
}

static uint8_t parse_json_number(const char *payload, const char *key, uint8_t *value)
{
  const char *cursor = find_json_key(payload, key);

  if ((cursor == 0) || (value == 0))
  {
    return 0;
  }

  cursor = strchr(cursor, ':');
  if (cursor == 0)
  {
    return 0;
  }

  cursor = skip_ws(cursor + 1);
  if ((*cursor < '0') || (*cursor > '9'))
  {
    return 0;
  }

  *value = (uint8_t)(*cursor - '0');
  return 1;
}

static uint8_t parse_json_target(const char *payload, LedTarget *target)
{
  const char *cursor = find_json_key(payload, "target");

  if ((cursor == 0) || (target == 0))
  {
    return 0;
  }

  cursor = strchr(cursor, ':');
  if (cursor == 0)
  {
    return 0;
  }

  cursor = skip_ws(cursor + 1);
  if (*cursor != '"')
  {
    return 0;
  }
  cursor++;

  if (strncmp(cursor, "A\"", 2) == 0)
  {
    *target = LED_TARGET_A;
    return 1;
  }
  if (strncmp(cursor, "B\"", 2) == 0)
  {
    *target = LED_TARGET_B;
    return 1;
  }
  if (strncmp(cursor, "ALL\"", 4) == 0)
  {
    *target = LED_TARGET_ALL;
    return 1;
  }

  return 0;
}

uint8_t LedCommand_Parse(const char *payload, LedCommand *command)
{
  LedCommand parsed;

  if ((payload == 0) || (command == 0))
  {
    return 0;
  }

  if (!parse_json_target(payload, &parsed.target) ||
      !parse_json_number(payload, "led", &parsed.led) ||
      !parse_json_number(payload, "state", &parsed.state))
  {
    return 0;
  }

  if ((parsed.led < 1U) || (parsed.led > 3U) || (parsed.state > 1U))
  {
    return 0;
  }

  *command = parsed;
  return 1;
}

const char *LedCommand_TargetString(LedTarget target)
{
  switch (target)
  {
    case LED_TARGET_A: return "A";
    case LED_TARGET_B: return "B";
    case LED_TARGET_ALL: return "ALL";
    default: return "?";
  }
}
