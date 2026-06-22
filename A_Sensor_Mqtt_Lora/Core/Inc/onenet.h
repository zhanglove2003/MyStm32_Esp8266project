#ifndef __ONENET_H__
#define __ONENET_H__

#include "main.h"
#include "dht12.h"
#include "led_command.h"
#include "mq135.h"

#define ONENET_OK 0
#define ONENET_ERR_TCP 1
#define ONENET_ERR_MQTT_BUILD 2
#define ONENET_ERR_MQTT_SEND 3
#define ONENET_ERR_CONNACK_TIMEOUT 4
#define ONENET_ERR_CONNACK_CODE 5
#define ONENET_ERR_PUBLISH_BUILD 6
#define ONENET_ERR_PUBLISH_SEND 7
#define ONENET_ERR_SUBSCRIBE_BUILD 8
#define ONENET_ERR_SUBSCRIBE_SEND 9
#define ONENET_ERR_NO_COMMAND 10
#define ONENET_ERR_COMMAND_PACKET 11

uint8_t OneNet_Connect(void);
uint8_t OneNet_SubscribeCommands(void);
uint8_t OneNet_SendData(const DHT12_Data *data, const MQ135_Data *gas);
uint8_t OneNet_PollCommand(LedCommand *command);
const char *OneNet_StatusString(uint8_t status);
uint8_t OneNet_LastConnAckCode(void);

#endif
