#include "mq135.h"
#include "adc.h"

#include <math.h>
#include <string.h>

static float mq135_r0_kohm = MQ135_R0_DEFAULT_KOHM;
static uint8_t mq135_calibrated = 0;

static MQ135_Level mq135_level_from_ppm(float ppm)
{
  if (ppm >= 2000.0f)
  {
    return MQ135_LEVEL_HIGH;
  }
  if (ppm >= 1000.0f)
  {
    return MQ135_LEVEL_WARN;
  }
  return MQ135_LEVEL_NORMAL;
}

static float mq135_rs_from_ao(float ao_voltage)
{
  if ((ao_voltage <= 0.01f) || (ao_voltage >= MQ135_SENSOR_VC))
  {
    return 0.0f;
  }

  return MQ135_LOAD_RES_KOHM * (MQ135_SENSOR_VC - ao_voltage) / ao_voltage;
}

static float mq135_co2_from_ratio(float ratio)
{
  if (ratio <= 0.0f)
  {
    return 0.0f;
  }

  return MQ135_CO2_CURVE_A * powf(ratio, MQ135_CO2_CURVE_B);
}

static float mq135_ratio_for_ppm(float ppm)
{
  if (ppm <= 0.0f)
  {
    return 0.0f;
  }

  return powf(ppm / MQ135_CO2_CURVE_A, 1.0f / MQ135_CO2_CURVE_B);
}

static HAL_StatusTypeDef mq135_read_raw(MQ135_Data *data)
{
  HAL_StatusTypeDef status;

  memset(data, 0, sizeof(*data));

  status = HAL_ADC_Start(&hadc1);
  if (status != HAL_OK)
  {
    data->hal_status = status;
    return status;
  }

  status = HAL_ADC_PollForConversion(&hadc1, 20);
  if (status == HAL_OK)
  {
    data->adc_raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
    data->pin_voltage = ((float)data->adc_raw * MQ135_VREF) / MQ135_ADC_MAX;
    data->ao_voltage = data->pin_voltage * MQ135_DIVIDER_RATIO;
    data->airq_percent = (data->ao_voltage * 100.0f) / MQ135_SENSOR_VC;
    data->percent = data->airq_percent;
    data->rs_kohm = mq135_rs_from_ao(data->ao_voltage);
  }

  HAL_ADC_Stop(&hadc1);
  data->hal_status = status;
  return status;
}

HAL_StatusTypeDef MQ135_Calibrate(void)
{
  MQ135_Data sample;
  HAL_StatusTypeDef status = HAL_OK;
  float rs_sum = 0.0f;
  float clean_air_ratio;
  uint32_t valid_samples = 0;

  IWDG->KR = 0xAAAA;
  HAL_Delay(MQ135_WARMUP_MS);

  for (uint32_t i = 0; i < MQ135_CALIBRATION_SAMPLES; i++)
  {
    IWDG->KR = 0xAAAA;
    status = mq135_read_raw(&sample);
    if (status != HAL_OK)
    {
      mq135_r0_kohm = MQ135_R0_DEFAULT_KOHM;
      mq135_calibrated = 0;
      return status;
    }

    if (sample.rs_kohm > 0.0f)
    {
      rs_sum += sample.rs_kohm;
      valid_samples++;
    }

    HAL_Delay(MQ135_CALIBRATION_DELAY_MS);
  }

  clean_air_ratio = mq135_ratio_for_ppm(MQ135_FRESH_AIR_PPM);
  if ((valid_samples == 0U) || (clean_air_ratio <= 0.0f))
  {
    mq135_r0_kohm = MQ135_R0_DEFAULT_KOHM;
    mq135_calibrated = 0;
    return HAL_ERROR;
  }

  mq135_r0_kohm = (rs_sum / (float)valid_samples) / clean_air_ratio;
  if (mq135_r0_kohm <= 0.0f)
  {
    mq135_r0_kohm = MQ135_R0_DEFAULT_KOHM;
    mq135_calibrated = 0;
    return HAL_ERROR;
  }

  mq135_calibrated = 1;
  return HAL_OK;
}

HAL_StatusTypeDef MQ135_Read(MQ135_Data *data)
{
  HAL_StatusTypeDef status = mq135_read_raw(data);

  if (status == HAL_OK)
  {
    data->r0_kohm = mq135_r0_kohm;
    data->calibrated = mq135_calibrated;

    if ((data->rs_kohm > 0.0f) && (mq135_r0_kohm > 0.0f))
    {
      data->co2_ppm = mq135_co2_from_ratio(data->rs_kohm / mq135_r0_kohm);
      data->co2_percent = data->co2_ppm / 10000.0f;
    }

    data->level = mq135_level_from_ppm(data->co2_ppm);
  }

  return status;
}

const char *MQ135_LevelString(MQ135_Level level)
{
  switch (level)
  {
    case MQ135_LEVEL_NORMAL: return "NORMAL";
    case MQ135_LEVEL_WARN: return "WARN";
    case MQ135_LEVEL_HIGH: return "HIGH";
    default: return "UNKNOWN";
  }
}

uint8_t MQ135_IsCalibrated(void)
{
  return mq135_calibrated;
}

float MQ135_GetR0(void)
{
  return mq135_r0_kohm;
}
