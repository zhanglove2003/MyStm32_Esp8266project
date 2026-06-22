#include "w25qxx.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;

#define W25QXX_CMD_JEDEC_ID 0x9F
#define W25QXX_CMD_WRITE_ENABLE 0x06
#define W25QXX_CMD_READ_STATUS1 0x05
#define W25QXX_CMD_SECTOR_ERASE 0x20
#define W25QXX_CMD_PAGE_PROGRAM 0x02
#define W25QXX_CMD_READ_DATA 0x03
#define W25QXX_SPI_TIMEOUT  100
#define W25QXX_BUSY_TIMEOUT_MS 3000

static void W25QXX_Select(void)
{
  HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(W25QXX_CS_GPIO_Port, W25QXX_CS_Pin, GPIO_PIN_RESET);
}

static void W25QXX_Unselect(void)
{
  HAL_GPIO_WritePin(W25QXX_CS_GPIO_Port, W25QXX_CS_Pin, GPIO_PIN_SET);
}

static void W25QXX_WriteEnable(void)
{
  uint8_t command = W25QXX_CMD_WRITE_ENABLE;

  W25QXX_Select();
  HAL_SPI_Transmit(&hspi1, &command, 1, W25QXX_SPI_TIMEOUT);
  W25QXX_Unselect();
}

static uint8_t W25QXX_ReadStatus1(void)
{
  uint8_t tx[2] = {W25QXX_CMD_READ_STATUS1, 0xFF};
  uint8_t rx[2] = {0};

  W25QXX_Select();
  HAL_SPI_TransmitReceive(&hspi1, tx, rx, sizeof(tx), W25QXX_SPI_TIMEOUT);
  W25QXX_Unselect();

  return rx[1];
}

static uint8_t W25QXX_WaitBusy(void)
{
  uint32_t start = HAL_GetTick();

  while ((W25QXX_ReadStatus1() & 0x01U) != 0U)
  {
    if ((HAL_GetTick() - start) > W25QXX_BUSY_TIMEOUT_MS)
    {
      return 0;
    }
  }

  return 1;
}

void W25QXX_ReadID(uint8_t id[3])
{
  uint8_t tx[4] = {W25QXX_CMD_JEDEC_ID, 0xFF, 0xFF, 0xFF};
  uint8_t rx[4] = {0};

  W25QXX_Select();
  if (HAL_SPI_TransmitReceive(&hspi1, tx, rx, sizeof(tx), W25QXX_SPI_TIMEOUT) == HAL_OK)
  {
    id[0] = rx[1];
    id[1] = rx[2];
    id[2] = rx[3];
  }
  else
  {
    id[0] = 0x00;
    id[1] = 0x00;
    id[2] = 0x00;
  }
  W25QXX_Unselect();
}

uint8_t W25QXX_IsValidID(const uint8_t id[3])
{
  uint8_t all_zero = (id[0] == 0x00U) && (id[1] == 0x00U) && (id[2] == 0x00U);
  uint8_t all_ff = (id[0] == 0xFFU) && (id[1] == 0xFFU) && (id[2] == 0xFFU);

  return (uint8_t)(!all_zero && !all_ff);
}

uint32_t W25QXX_GetCapacityBytes(const uint8_t id[3])
{
  if (!W25QXX_IsValidID(id) || (id[2] < 0x10U) || (id[2] > 0x20U))
  {
    return 0;
  }

  return (uint32_t)1UL << id[2];
}

uint8_t W25QXX_ReadData(uint32_t address, uint8_t *data, uint16_t length)
{
  uint8_t command[4];

  command[0] = W25QXX_CMD_READ_DATA;
  command[1] = (uint8_t)(address >> 16);
  command[2] = (uint8_t)(address >> 8);
  command[3] = (uint8_t)address;

  W25QXX_Select();
  if (HAL_SPI_Transmit(&hspi1, command, sizeof(command), W25QXX_SPI_TIMEOUT) != HAL_OK)
  {
    W25QXX_Unselect();
    return 0;
  }
  if (HAL_SPI_Receive(&hspi1, data, length, W25QXX_SPI_TIMEOUT) != HAL_OK)
  {
    W25QXX_Unselect();
    return 0;
  }
  W25QXX_Unselect();

  return 1;
}

uint8_t W25QXX_PageProgram(uint32_t address, const uint8_t *data, uint16_t length)
{
  uint8_t command[4];

  if ((length == 0U) || (length > 256U))
  {
    return 0;
  }

  W25QXX_WriteEnable();

  command[0] = W25QXX_CMD_PAGE_PROGRAM;
  command[1] = (uint8_t)(address >> 16);
  command[2] = (uint8_t)(address >> 8);
  command[3] = (uint8_t)address;

  W25QXX_Select();
  if (HAL_SPI_Transmit(&hspi1, command, sizeof(command), W25QXX_SPI_TIMEOUT) != HAL_OK)
  {
    W25QXX_Unselect();
    return 0;
  }
  if (HAL_SPI_Transmit(&hspi1, (uint8_t *)data, length, W25QXX_SPI_TIMEOUT) != HAL_OK)
  {
    W25QXX_Unselect();
    return 0;
  }
  W25QXX_Unselect();

  return W25QXX_WaitBusy();
}

uint8_t W25QXX_SectorErase(uint32_t address)
{
  uint8_t command[4];

  W25QXX_WriteEnable();

  command[0] = W25QXX_CMD_SECTOR_ERASE;
  command[1] = (uint8_t)(address >> 16);
  command[2] = (uint8_t)(address >> 8);
  command[3] = (uint8_t)address;

  W25QXX_Select();
  if (HAL_SPI_Transmit(&hspi1, command, sizeof(command), W25QXX_SPI_TIMEOUT) != HAL_OK)
  {
    W25QXX_Unselect();
    return 0;
  }
  W25QXX_Unselect();

  return W25QXX_WaitBusy();
}
