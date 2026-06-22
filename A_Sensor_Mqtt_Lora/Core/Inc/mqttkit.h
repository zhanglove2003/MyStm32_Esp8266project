#ifndef __MQTTKIT_H__
#define __MQTTKIT_H__

#include <stdint.h>

#define MQTT_OK 0
#define MQTT_ERR_BUFFER 1
#define MQTT_ERR_PACKET 2

uint8_t MQTT_BuildConnect(const char *client_id, const char *username, const char *password,
                          uint16_t keep_alive, uint8_t clean_session,
                          uint8_t *out, uint16_t out_size, uint16_t *out_len);
uint8_t MQTT_ParseConnAck(const uint8_t *packet, uint16_t len, uint8_t *return_code);
uint8_t MQTT_BuildPublish(const char *topic, const char *payload,
                          uint8_t *out, uint16_t out_size, uint16_t *out_len);
uint8_t MQTT_BuildSubscribe(uint16_t packet_id, const char *topic,
                            uint8_t *out, uint16_t out_size, uint16_t *out_len);
uint8_t MQTT_ParsePublish(const uint8_t *packet, uint16_t len,
                          char *topic, uint16_t topic_size,
                          char *payload, uint16_t payload_size);
uint8_t MQTT_ParsePublishPayload(const uint8_t *packet, uint16_t len,
                                 char *payload, uint16_t payload_size);

#endif
