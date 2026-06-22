#ifndef __MQ135_H__
#define __MQ135_H__

#include "main.h"

#define MQ135_ADC_MAX           4095.0f
#define MQ135_VREF             3.3f
#define MQ135_SENSOR_VC        5.0f
#define MQ135_DIVIDER_RATIO    2.0f
#define MQ135_LOAD_RES_KOHM    10.0f
#define MQ135_FRESH_AIR_PPM    400.0f
#define MQ135_CO2_CURVE_A      116.6020682f
#define MQ135_CO2_CURVE_B      -2.769034857f
#define MQ135_WARMUP_MS        3000U
#define MQ135_CALIBRATION_SAMPLES 20U
#define MQ135_CALIBRATION_DELAY_MS 50U
#define MQ135_R0_DEFAULT_KOHM  76.63f

typedef enum {
  MQ135_LEVEL_NORMAL = 0,
  MQ135_LEVEL_WARN = 1,
  MQ135_LEVEL_HIGH = 2
} MQ135_Level;

typedef struct {
  uint16_t adc_raw;
  float pin_voltage;
  float ao_voltage;
  float percent;
  float airq_percent;
  float rs_kohm;
  float r0_kohm;
  float co2_ppm;
  float co2_percent;
  uint8_t calibrated;
  MQ135_Level level;
  HAL_StatusTypeDef hal_status;
} MQ135_Data;

HAL_StatusTypeDef MQ135_Calibrate(void);
HAL_StatusTypeDef MQ135_Read(MQ135_Data *data);
const char *MQ135_LevelString(MQ135_Level level);
uint8_t MQ135_IsCalibrated(void);
float MQ135_GetR0(void);

#endif /* __MQ135_H__ */
