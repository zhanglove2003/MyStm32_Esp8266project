#include <stdio.h>
#include <string.h>

#include "../A_Sensor_Mqtt_Lora/Core/Inc/control_frame.h"
#include "../A_Sensor_Mqtt_Lora/Core/Inc/led_command.h"
#include "../A_Sensor_Mqtt_Lora/Core/Inc/mqttkit.h"

static int failures;

static void expect_int(const char *name, int actual, int expected)
{
  if (actual != expected)
  {
    printf("FAIL %s: got %d expected %d\n", name, actual, expected);
    failures++;
  }
}

static void expect_string(const char *name, const char *actual, const char *expected)
{
  if (strcmp(actual, expected) != 0)
  {
    printf("FAIL %s: got '%s' expected '%s'\n", name, actual, expected);
    failures++;
  }
}

static void test_cloud_command_target_a(void)
{
  LedCommand command;
  expect_int("parse target A", LedCommand_Parse("{\"target\":\"A\",\"led\":1,\"state\":1}", &command), 1);
  expect_int("target A", command.target, LED_TARGET_A);
  expect_int("led 1", command.led, 1);
  expect_int("state on", command.state, 1);
}

static void test_cloud_command_target_all_with_spacing(void)
{
  LedCommand command;
  expect_int("parse target ALL", LedCommand_Parse("{ \"target\" : \"ALL\", \"led\" : 3, \"state\" : 0 }", &command), 1);
  expect_int("target ALL", command.target, LED_TARGET_ALL);
  expect_int("led 3", command.led, 3);
  expect_int("state off", command.state, 0);
}

static void test_cloud_command_rejects_bad_led(void)
{
  LedCommand command;
  expect_int("reject led 4", LedCommand_Parse("{\"target\":\"B\",\"led\":4,\"state\":1}", &command), 0);
}

static void test_build_led_frame(void)
{
  char frame[16];
  expect_int("build frame", ControlFrame_BuildLed(2, 1, frame, sizeof(frame)), 1);
  expect_string("frame text", frame, "$L=2,S=1#");
}

static void test_parse_led_frame_stream(void)
{
  ControlFrameParser parser;
  LedControlFrame frame;
  const char *text = "noise$L=1,S=1#$T=25.6,H=60.2,C=780#";
  int got = 0;

  ControlFrameParser_Init(&parser);
  for (size_t i = 0; text[i] != '\0'; i++)
  {
    if (ControlFrameParser_ProcessByte(&parser, (unsigned char)text[i], &frame))
    {
      got = 1;
      break;
    }
  }

  expect_int("parse frame", got, 1);
  expect_int("frame led", frame.led, 1);
  expect_int("frame state", frame.state, 1);
}

static void test_build_mqtt_pingreq(void)
{
  unsigned char packet[2] = {0};
  unsigned short len = 0;

  expect_int("build pingreq status", MQTT_BuildPingReq(packet, sizeof(packet), &len), MQTT_OK);
  expect_int("pingreq len", len, 2);
  expect_int("pingreq header", packet[0], 0xC0);
  expect_int("pingreq remaining", packet[1], 0x00);
}

int main(void)
{
  test_cloud_command_target_a();
  test_cloud_command_target_all_with_spacing();
  test_cloud_command_rejects_bad_led();
  test_build_led_frame();
  test_parse_led_frame_stream();
  test_build_mqtt_pingreq();

  if (failures != 0)
  {
    printf("%d failure(s)\n", failures);
    return 1;
  }

  printf("all tests passed\n");
  return 0;
}
