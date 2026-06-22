#include "mqttkit.h"
#include <string.h>

static uint8_t put_string(uint8_t *buf, uint16_t size, uint16_t *pos, const char *text)
{
  uint16_t len = (uint16_t)strlen(text);

  if ((uint32_t)(*pos) + len + 2 > size)
    return MQTT_ERR_BUFFER;

  buf[(*pos)++] = (uint8_t)(len >> 8);
  buf[(*pos)++] = (uint8_t)(len & 0xFF);
  memcpy(&buf[*pos], text, len);
  *pos += len;
  return MQTT_OK;
}

static uint8_t put_remaining_length(uint8_t *buf, uint16_t size, uint16_t *pos, uint32_t len)
{
  do
  {
    uint8_t encoded = (uint8_t)(len % 128);
    len /= 128;
    if (len > 0)
      encoded |= 128;
    if (*pos >= size)
      return MQTT_ERR_BUFFER;
    buf[(*pos)++] = encoded;
  } while (len > 0);

  return MQTT_OK;
}

uint8_t MQTT_BuildConnect(const char *client_id, const char *username, const char *password,
                          uint16_t keep_alive, uint8_t clean_session,
                          uint8_t *out, uint16_t out_size, uint16_t *out_len)
{
  uint16_t pos = 0;
  uint32_t remain_len;
  uint8_t flags = 0;

  remain_len = 10 + 2 + strlen(client_id) + 2 + strlen(username) + 2 + strlen(password);
  flags |= 0x80;
  flags |= 0x40;
  if (clean_session)
    flags |= 0x02;

  if (pos >= out_size) return MQTT_ERR_BUFFER;
  out[pos++] = 0x10;
  if (put_remaining_length(out, out_size, &pos, remain_len) != MQTT_OK) return MQTT_ERR_BUFFER;
  if (put_string(out, out_size, &pos, "MQTT") != MQTT_OK) return MQTT_ERR_BUFFER;

  if ((uint32_t)pos + 4 > out_size) return MQTT_ERR_BUFFER;
  out[pos++] = 0x04;
  out[pos++] = flags;
  out[pos++] = (uint8_t)(keep_alive >> 8);
  out[pos++] = (uint8_t)(keep_alive & 0xFF);

  if (put_string(out, out_size, &pos, client_id) != MQTT_OK) return MQTT_ERR_BUFFER;
  if (put_string(out, out_size, &pos, username) != MQTT_OK) return MQTT_ERR_BUFFER;
  if (put_string(out, out_size, &pos, password) != MQTT_OK) return MQTT_ERR_BUFFER;

  *out_len = pos;
  return MQTT_OK;
}

uint8_t MQTT_ParseConnAck(const uint8_t *packet, uint16_t len, uint8_t *return_code)
{
  if (len < 4 || packet[0] != 0x20 || packet[1] != 0x02)
    return MQTT_ERR_PACKET;

  *return_code = packet[3];
  return MQTT_OK;
}

uint8_t MQTT_BuildPublish(const char *topic, const char *payload,
                          uint8_t *out, uint16_t out_size, uint16_t *out_len)
{
  uint16_t pos = 0;
  uint16_t topic_len = (uint16_t)strlen(topic);
  uint16_t payload_len = (uint16_t)strlen(payload);
  uint32_t remain_len = 2 + topic_len + payload_len;

  if (pos >= out_size) return MQTT_ERR_BUFFER;
  out[pos++] = 0x30;
  if (put_remaining_length(out, out_size, &pos, remain_len) != MQTT_OK) return MQTT_ERR_BUFFER;

  if ((uint32_t)pos + 2 + topic_len + payload_len > out_size) return MQTT_ERR_BUFFER;
  out[pos++] = (uint8_t)(topic_len >> 8);
  out[pos++] = (uint8_t)(topic_len & 0xFF);
  memcpy(&out[pos], topic, topic_len);
  pos += topic_len;
  memcpy(&out[pos], payload, payload_len);
  pos += payload_len;

  *out_len = pos;
  return MQTT_OK;
}

uint8_t MQTT_BuildSubscribe(uint16_t packet_id, const char *topic,
                            uint8_t *out, uint16_t out_size, uint16_t *out_len)
{
  uint16_t pos = 0;
  uint16_t topic_len = (uint16_t)strlen(topic);
  uint32_t remain_len = 2U + 2U + topic_len + 1U;

  if (pos >= out_size) return MQTT_ERR_BUFFER;
  out[pos++] = 0x82;
  if (put_remaining_length(out, out_size, &pos, remain_len) != MQTT_OK) return MQTT_ERR_BUFFER;

  if ((uint32_t)pos + 2U + 2U + topic_len + 1U > out_size) return MQTT_ERR_BUFFER;
  out[pos++] = (uint8_t)(packet_id >> 8);
  out[pos++] = (uint8_t)(packet_id & 0xFF);
  out[pos++] = (uint8_t)(topic_len >> 8);
  out[pos++] = (uint8_t)(topic_len & 0xFF);
  memcpy(&out[pos], topic, topic_len);
  pos += topic_len;
  out[pos++] = 0x00;

  *out_len = pos;
  return MQTT_OK;
}

static uint8_t parse_remaining_length(const uint8_t *packet, uint16_t len, uint16_t *pos, uint32_t *remaining)
{
  uint32_t multiplier = 1;
  uint32_t value = 0;
  uint8_t encoded;

  *pos = 1;
  do
  {
    if (*pos >= len)
    {
      return MQTT_ERR_PACKET;
    }
    encoded = packet[(*pos)++];
    value += (uint32_t)(encoded & 127U) * multiplier;
    multiplier *= 128U;
    if (multiplier > (128U * 128U * 128U))
    {
      return MQTT_ERR_PACKET;
    }
  } while ((encoded & 128U) != 0U);

  *remaining = value;
  return MQTT_OK;
}

uint8_t MQTT_ParsePublish(const uint8_t *packet, uint16_t len,
                          char *topic, uint16_t topic_size,
                          char *payload, uint16_t payload_size)
{
  uint16_t pos;
  uint32_t remaining;
  uint16_t end;
  uint16_t topic_len;
  uint16_t payload_len;
  uint16_t copy_len;

  if ((packet == 0) || (topic == 0) || (topic_size == 0U) ||
      (payload == 0) || (payload_size == 0U) ||
      ((packet[0] & 0xF0U) != 0x30U))
  {
    return MQTT_ERR_PACKET;
  }

  if (parse_remaining_length(packet, len, &pos, &remaining) != MQTT_OK)
  {
    return MQTT_ERR_PACKET;
  }

  end = (uint16_t)((uint32_t)pos + remaining);
  if ((end > len) || ((uint32_t)pos + 2U > end))
  {
    return MQTT_ERR_PACKET;
  }

  topic_len = (uint16_t)(((uint16_t)packet[pos] << 8) | packet[pos + 1U]);
  pos += 2U;
  if ((uint32_t)pos + topic_len > end)
  {
    return MQTT_ERR_PACKET;
  }

  copy_len = topic_len;
  if (copy_len >= topic_size)
  {
    copy_len = (uint16_t)(topic_size - 1U);
  }
  memcpy(topic, &packet[pos], copy_len);
  topic[copy_len] = '\0';
  pos = (uint16_t)(pos + topic_len);

  if ((packet[0] & 0x06U) != 0U)
  {
    if ((uint32_t)pos + 2U > end)
    {
      return MQTT_ERR_PACKET;
    }
    pos += 2U;
  }

  payload_len = (uint16_t)(end - pos);
  if (payload_len >= payload_size)
  {
    payload_len = (uint16_t)(payload_size - 1U);
  }
  memcpy(payload, &packet[pos], payload_len);
  payload[payload_len] = '\0';

  return MQTT_OK;
}

uint8_t MQTT_ParsePublishPayload(const uint8_t *packet, uint16_t len,
                                 char *payload, uint16_t payload_size)
{
  char topic[1];

  return MQTT_ParsePublish(packet, len, topic, sizeof(topic), payload, payload_size);
}
