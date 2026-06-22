#include "uart_loader.h"
#include "boot_anim.h"
#include "tft.h"
#include "w25qxx.h"
#include <string.h>

#define UART_LOADER_XMODEM_128_BLOCK_SIZE 128U
#define UART_LOADER_XMODEM_1K_BLOCK_SIZE 1024U
#define UART_LOADER_SECTOR_SIZE 4096UL
#define W25QXX_PAGE_SIZE 256U
#define UART_LOADER_CONFIRM_TIMEOUT 5000U
#define UART_LOADER_RX_TIMEOUT 5000U
#define UART_LOADER_MAX_SIZE (2UL * 1024UL * 1024UL)

#define XMODEM_SOH 0x01U
#define XMODEM_STX 0x02U
#define XMODEM_EOT 0x04U
#define XMODEM_ACK 0x06U
#define XMODEM_NAK 0x15U
#define XMODEM_CAN 0x18U
#define XMODEM_CRCCHR 'C'

static void uart_send_text(UART_HandleTypeDef *huart, const char *text)
{
  HAL_UART_Transmit(huart, (uint8_t *)text, (uint16_t)strlen(text), 1000);
}

static void uart_send_byte(UART_HandleTypeDef *huart, uint8_t value)
{
  HAL_UART_Transmit(huart, &value, 1, 1000);
}

static uint8_t ascii_lower(uint8_t ch)
{
  if ((ch >= 'A') && (ch <= 'Z'))
  {
    return (uint8_t)(ch + ('a' - 'A'));
  }
  return ch;
}

static uint8_t wait_download_confirm(UART_HandleTypeDef *huart)
{
  uint8_t input[8] = {0};
  uint8_t length = 0;
  uint32_t start = HAL_GetTick();

  uart_send_text(huart, "start download\r\n");

  while ((HAL_GetTick() - start) < UART_LOADER_CONFIRM_TIMEOUT)
  {
    uint8_t ch;
    if (HAL_UART_Receive(huart, &ch, 1, 50) != HAL_OK)
    {
      continue;
    }

    if ((ch == '\r') || (ch == '\n'))
    {
      if (length == 0)
      {
        continue;
      }
      break;
    }

    if (length < (sizeof(input) - 1U))
    {
      input[length++] = ascii_lower(ch);
      input[length] = 0;
    }

    if ((length == 3U) &&
        (input[0] == 'y') &&
        (input[1] == 'e') &&
        (input[2] == 's'))
    {
      return 1;
    }
  }

  return (uint8_t)((length == 3U) &&
                   (input[0] == 'y') &&
                   (input[1] == 'e') &&
                   (input[2] == 's'));
}

static uint8_t receive_exact(UART_HandleTypeDef *huart, uint8_t *data, uint16_t length, uint32_t timeout)
{
  return (uint8_t)(HAL_UART_Receive(huart, data, length, timeout) == HAL_OK);
}

static uint16_t xmodem_crc16(const uint8_t *data, uint16_t length)
{
  uint16_t crc = 0;

  while (length--)
  {
    crc ^= (uint16_t)(*data++) << 8;
    for (uint8_t bit = 0; bit < 8U; bit++)
    {
      if (crc & 0x8000U)
      {
        crc = (uint16_t)((crc << 1) ^ 0x1021U);
      }
      else
      {
        crc <<= 1;
      }
    }
  }

  return crc;
}

static uint8_t erase_sector_for_offset(uint32_t offset)
{
  if ((offset % UART_LOADER_SECTOR_SIZE) != 0U)
  {
    return 1;
  }

  return W25QXX_SectorErase(BOOT_ANIM_ADDRESS + offset);
}

static uint8_t program_flash_block(uint32_t offset, const uint8_t *data, uint16_t length)
{
  uint16_t programmed = 0;

  while (programmed < length)
  {
    uint16_t chunk = (uint16_t)(length - programmed);
    if (chunk > W25QXX_PAGE_SIZE)
    {
      chunk = W25QXX_PAGE_SIZE;
    }

    if (!W25QXX_PageProgram(BOOT_ANIM_ADDRESS + offset + programmed,
                            &data[programmed],
                            chunk))
    {
      return 0;
    }

    programmed = (uint16_t)(programmed + chunk);
  }

  return 1;
}

static uint8_t receive_xmodem_to_flash(UART_HandleTypeDef *huart)
{
  uint8_t header[2];
  uint8_t data[UART_LOADER_XMODEM_1K_BLOCK_SIZE];
  uint8_t crc_bytes[2];
  uint8_t expected_packet = 1;
  uint32_t written = 0;
  uint8_t retries = 0;

  uart_send_byte(huart, XMODEM_CRCCHR);

  while (1)
  {
    uint8_t lead = 0;

    if (HAL_UART_Receive(huart, &lead, 1, UART_LOADER_RX_TIMEOUT) != HAL_OK)
    {
      if (++retries > 10U)
      {
        uart_send_byte(huart, XMODEM_CAN);
        uart_send_text(huart, "\nERR RX\n");
        return 0;
      }
      uart_send_byte(huart, XMODEM_CRCCHR);
      continue;
    }

    retries = 0;

    if (lead == XMODEM_EOT)
    {
      uart_send_byte(huart, XMODEM_ACK);
      uart_send_text(huart, "\nOK DONE\n");
      return 1;
    }

    if (lead == XMODEM_CAN)
    {
      uart_send_text(huart, "\nERR CAN\n");
      return 0;
    }

    uint16_t block_size;
    if (lead == XMODEM_SOH)
    {
      block_size = UART_LOADER_XMODEM_128_BLOCK_SIZE;
    }
    else if (lead == XMODEM_STX)
    {
      block_size = UART_LOADER_XMODEM_1K_BLOCK_SIZE;
    }
    else
    {
      uart_send_byte(huart, XMODEM_NAK);
      continue;
    }

    if (!receive_exact(huart, header, sizeof(header), UART_LOADER_RX_TIMEOUT) ||
        !receive_exact(huart, data, block_size, UART_LOADER_RX_TIMEOUT) ||
        !receive_exact(huart, crc_bytes, sizeof(crc_bytes), UART_LOADER_RX_TIMEOUT))
    {
      uart_send_byte(huart, XMODEM_NAK);
      continue;
    }

    uint8_t packet = header[0];
    uint8_t packet_inv = header[1];
    uint16_t received_crc = ((uint16_t)crc_bytes[0] << 8) | crc_bytes[1];
    uint16_t calculated_crc = xmodem_crc16(data, block_size);

    if (((uint8_t)(packet + packet_inv) != 0xFFU) || (received_crc != calculated_crc))
    {
      uart_send_byte(huart, XMODEM_NAK);
      continue;
    }

    if (packet == (uint8_t)(expected_packet - 1U))
    {
      uart_send_byte(huart, XMODEM_ACK);
      continue;
    }

    if (packet != expected_packet)
    {
      uart_send_byte(huart, XMODEM_CAN);
      uart_send_text(huart, "\nERR SEQ\n");
      return 0;
    }

    if ((written + block_size) > UART_LOADER_MAX_SIZE)
    {
      uart_send_byte(huart, XMODEM_CAN);
      uart_send_text(huart, "\nERR RANGE\n");
      return 0;
    }

    if (!erase_sector_for_offset(written))
    {
      uart_send_byte(huart, XMODEM_CAN);
      uart_send_text(huart, "\nERR ERASE\n");
      return 0;
    }

    if (!program_flash_block(written, data, block_size))
    {
      uart_send_byte(huart, XMODEM_CAN);
      uart_send_text(huart, "\nERR WRITE\n");
      return 0;
    }

    written += block_size;
    expected_packet++;
    uart_send_byte(huart, XMODEM_ACK);
  }
}

uint8_t UART_LOADER_Run(UART_HandleTypeDef *huart)
{
  if (!wait_download_confirm(huart))
  {
    return 0;
  }
  uart_send_text(huart, "ok\r\n");

  TFT_FillScreen(TFT_BLACK);
  TFT_DrawString(4, 6, "UART LOAD", TFT_WHITE, TFT_BLACK);
  uart_send_text(huart, "XMODEM READY\r\n");

  if (!receive_xmodem_to_flash(huart))
  {
    return 1;
  }

  TFT_FillScreen(TFT_BLACK);
  TFT_DrawString(4, 6, "LOAD DONE", TFT_GREEN, TFT_BLACK);
  return 1;
}
