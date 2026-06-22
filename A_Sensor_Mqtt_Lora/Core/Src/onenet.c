#include "onenet.h"
#include "onenet_config.h"
#include "esp8266.h"
#include "mqttkit.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

static uint8_t onenet_connack_code = 0xFF;
static char onenet_log_msg[260];
static char onenet_post_topic[96];
static char onenet_post_payload[240];
static char onenet_post_rx_topic[120];
static char onenet_post_rx_payload[180];
static uint8_t onenet_post_packet[320];
static uint8_t onenet_post_ipd[300];
static char onenet_poll_topic[120];
static char onenet_poll_payload[180];
static uint8_t onenet_poll_ipd[256];

static void build_topic(char *topic, uint16_t topic_size, const char *suffix)
{
  snprintf(topic, topic_size, "$sys/%s/%s/%s",
           ONENET_PRODUCT_ID, ONENET_DEVICE_NAME, suffix);
}

static void debug_print(const char *text)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)text, strlen(text), 100);
}

uint8_t OneNet_LastConnAckCode(void)
{
  return onenet_connack_code;
}

uint8_t OneNet_Connect(void)
{
  uint8_t packet[360];
  uint8_t ipd[64];
  uint16_t packet_len = 0;
  uint16_t ipd_len = 0;
  uint8_t ret;
  char msg[128];

  ESP8266_CloseTcp();

  debug_print("OneNet: opening TCP\r\n");
  if (ESP8266_OpenTcp(ONENET_MQTT_HOST, ONENET_MQTT_PORT) != ESP8266_OK)
  {
    sprintf(msg, "OneNet TCP failed: %s\r\n", ESP8266_LastResponse());
    debug_print(msg);
    return ONENET_ERR_TCP;
  }

  debug_print("OneNet: sending MQTT CONNECT\r\n");
  ret = MQTT_BuildConnect(ONENET_DEVICE_NAME, ONENET_PRODUCT_ID, ONENET_AUTH_INFO,
                          256, 1, packet, sizeof(packet), &packet_len);
  if (ret != MQTT_OK)
    return ONENET_ERR_MQTT_BUILD;

  ESP8266_Clear();
  if (ESP8266_SendData(packet, packet_len, 5000) != ESP8266_OK)
    return ONENET_ERR_MQTT_SEND;

  if (ESP8266_WaitIPD(ipd, sizeof(ipd), &ipd_len, 5000) != ESP8266_OK)
    return ONENET_ERR_CONNACK_TIMEOUT;

  if (MQTT_ParseConnAck(ipd, ipd_len, &onenet_connack_code) != MQTT_OK)
    return ONENET_ERR_CONNACK_TIMEOUT;

  sprintf(msg, "OneNet MQTT CONNACK: 0x%02X\r\n", onenet_connack_code);
  debug_print(msg);

  if (onenet_connack_code != 0)
    return ONENET_ERR_CONNACK_CODE;

  return ONENET_OK;
}

uint8_t OneNet_SubscribeCommands(void)
{
  char topic[96];
  uint8_t packet[180];
  uint16_t packet_len = 0;

  build_topic(topic, sizeof(topic), "thing/property/set");
  if (MQTT_BuildSubscribe(1, topic, packet, sizeof(packet), &packet_len) != MQTT_OK)
    return ONENET_ERR_SUBSCRIBE_BUILD;

  if (ESP8266_SendData(packet, packet_len, 5000) != ESP8266_OK)
    return ONENET_ERR_SUBSCRIBE_SEND;

  debug_print("OneNet subscribe property set OK\r\n");

  build_topic(topic, sizeof(topic), "thing/property/post/reply");
  if (MQTT_BuildSubscribe(2, topic, packet, sizeof(packet), &packet_len) != MQTT_OK)
    return ONENET_ERR_SUBSCRIBE_BUILD;

  if (ESP8266_SendData(packet, packet_len, 5000) != ESP8266_OK)
    return ONENET_ERR_SUBSCRIBE_SEND;

  debug_print("OneNet subscribe property post reply OK\r\n");
  return ONENET_OK;
}

uint8_t OneNet_SendData(const DHT12_Data *data, const MQ135_Data *gas)
{
  uint16_t packet_len = 0;
  uint16_t ipd_len = 0;
  uint8_t ret;
  int temp_value;
  float co2_value = 0.0f;

  build_topic(onenet_post_topic, sizeof(onenet_post_topic), "thing/property/post");
  temp_value = (int)(data->temperature + 0.5f);
  if (gas != 0)
    co2_value = gas->co2_ppm / 100.0f; // Scale 400ppm to 4.0 to fit 0-100 range constraint

  sprintf(onenet_post_payload,
          "{\"id\":\"0\",\"version\":\"1.0\",\"params\":{\"%s\":{\"value\":%.1f},\"%s\":{\"value\":%d},\"%s\":{\"value\":%.1f}}}",
          ONENET_HUM_IDENTIFIER, data->humidity,
          ONENET_TEMP_IDENTIFIER, temp_value,
          ONENET_CO2_IDENTIFIER, co2_value);

  sprintf(onenet_log_msg, "POST topic=%s\r\n", onenet_post_topic);
  debug_print(onenet_log_msg);
  sprintf(onenet_log_msg, "POST payload=%s\r\n", onenet_post_payload);
  debug_print(onenet_log_msg);
  sprintf(onenet_log_msg, "POST CO2 upload=%.1f\r\n", co2_value);
  debug_print(onenet_log_msg);

  ret = MQTT_BuildPublish(onenet_post_topic, onenet_post_payload,
                          onenet_post_packet, sizeof(onenet_post_packet), &packet_len);
  if (ret != MQTT_OK)
    return ONENET_ERR_PUBLISH_BUILD;

  if (ESP8266_SendData(onenet_post_packet, packet_len, 5000) != ESP8266_OK)
  {
    sprintf(onenet_log_msg, "OneNet publish send resp=%s\r\n", ESP8266_LastResponse());
    debug_print(onenet_log_msg);
    return ONENET_ERR_PUBLISH_SEND;
  }

  if (ESP8266_WaitIPD(onenet_post_ipd, sizeof(onenet_post_ipd), &ipd_len, 3000) == ESP8266_OK)
  {
    if (MQTT_ParsePublish(onenet_post_ipd, ipd_len,
                          onenet_post_rx_topic, sizeof(onenet_post_rx_topic),
                          onenet_post_rx_payload, sizeof(onenet_post_rx_payload)) == MQTT_OK)
    {
      sprintf(onenet_log_msg, "POST reply topic=%s\r\n", onenet_post_rx_topic);
      debug_print(onenet_log_msg);
      sprintf(onenet_log_msg, "POST reply=%s\r\n", onenet_post_rx_payload);
      debug_print(onenet_log_msg);
    }
    else
    {
      debug_print("POST reply parse failed\r\n");
    }
  }
  else
  {
    debug_print("OneNet post reply timeout\r\n");
  }

  sprintf(onenet_log_msg, "OneNet publish OK, %u bytes\r\n", packet_len);
  debug_print(onenet_log_msg);
  return ONENET_OK;
}

uint8_t OneNet_PollCommand(LedCommand *command)
{
  uint16_t ipd_len = 0;

  if (ESP8266_WaitIPD(onenet_poll_ipd, sizeof(onenet_poll_ipd), &ipd_len, 20) != ESP8266_OK)
    return ONENET_ERR_NO_COMMAND;

  if (MQTT_ParsePublish(onenet_poll_ipd, ipd_len,
                        onenet_poll_topic, sizeof(onenet_poll_topic),
                        onenet_poll_payload, sizeof(onenet_poll_payload)) != MQTT_OK)
    return ONENET_ERR_COMMAND_PACKET;

  if (strstr(onenet_poll_topic, "thing/property/post/reply") != 0)
  {
    sprintf(onenet_log_msg, "POST reply late=%s\r\n", onenet_poll_payload);
    debug_print(onenet_log_msg);
    return ONENET_ERR_NO_COMMAND;
  }

  if (strstr(onenet_poll_topic, "thing/property/set") == 0)
  {
    sprintf(onenet_log_msg, "OneNet publish ignored topic=%s\r\n", onenet_poll_topic);
    debug_print(onenet_log_msg);
    return ONENET_ERR_NO_COMMAND;
  }

  if (!LedCommand_Parse(onenet_poll_payload, command))
    return ONENET_ERR_COMMAND_PACKET;

  return ONENET_OK;
}

const char *OneNet_StatusString(uint8_t status)
{
  switch (status)
  {
    case ONENET_OK: return "OK";
    case ONENET_ERR_TCP: return "TCP_FAILED";
    case ONENET_ERR_MQTT_BUILD: return "MQTT_BUILD_FAILED";
    case ONENET_ERR_MQTT_SEND: return "MQTT_SEND_FAILED";
    case ONENET_ERR_CONNACK_TIMEOUT: return "CONNACK_TIMEOUT";
    case ONENET_ERR_CONNACK_CODE: return "CONNACK_FAILED";
    case ONENET_ERR_PUBLISH_BUILD: return "PUBLISH_BUILD_FAILED";
    case ONENET_ERR_PUBLISH_SEND: return "PUBLISH_SEND_FAILED";
    case ONENET_ERR_SUBSCRIBE_BUILD: return "SUBSCRIBE_BUILD_FAILED";
    case ONENET_ERR_SUBSCRIBE_SEND: return "SUBSCRIBE_SEND_FAILED";
    case ONENET_ERR_NO_COMMAND: return "NO_COMMAND";
    case ONENET_ERR_COMMAND_PACKET: return "COMMAND_PACKET";
    default: return "UNKNOWN";
  }
}
