/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "esp8266.h"
#include "gps.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  WIFI_MONITOR_IDLE = 0,
  WIFI_MONITOR_CHECK_MODULE,
  WIFI_MONITOR_SET_STATION,
  WIFI_MONITOR_CONNECTING,
  WIFI_MONITOR_CONNECTED,
  WIFI_MONITOR_FAILED
} WiFiMonitorState;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* ESP8266 联网参数：这里先直接写死，后续可以改成从 Flash 或配置区读取。 */
#define WIFI_SSID "TP-LINK_DE1F"
#define WIFI_PASSWORD "fgw00000"
#define API_HOST "192.168.1.109"
#define API_PATH "/api/telemetry"
#define API_PORT 3000
#define GPS_UPLOAD_INTERVAL_MS 10000U
#define WIFI_RECONNECT_INTERVAL_MS 30000U
#define ULTRASONIC_TRIGGER_INTERVAL_MS 200U
#define ULTRASONIC_ECHO_TIMEOUT_MS 60U
#define ULTRASONIC_TIMER_PERIOD_US 65536U
#define MPU6050_ADDR (0x68U << 1)
#define MPU6050_REG_WHO_AM_I 0x75U
#define MPU6050_REG_PWR_MGMT_1 0x6BU
#define MPU6050_REG_ACCEL_XOUT_H 0x3BU
#define MPU6050_SAMPLE_INTERVAL_MS 100U
#define MPU6050_TILT_THRESHOLD_RAW 8192
#define MPU6050_ACCEL_MOVE_DELTA_RAW 6000
#define MPU6050_GYRO_MOVE_THRESHOLD_RAW 8000
#define MPU6050_ONE_G_RAW 16384
#define MPU6050_MAX_TILT_MDEG 90000
#define MPU6050_POWER_ON_DELAY_MS 500U
#define MPU6050_INIT_RETRY_INTERVAL_MS 1000U
#define MQ2_SAMPLE_INTERVAL_MS 200U
#define MQ2_AO_ADC_CHANNEL ADC_CHANNEL_4
#define MQ2_DEFAULT_ADC_CHANNEL ADC_CHANNEL_5
#define MQ2_AO_ALARM_THRESHOLD 2000U
#define MQ2_DO_ALARM_STATE GPIO_PIN_RESET
#define ALARM_LED_ACTIVE_STATE GPIO_PIN_SET
#define ALARM_LED_INACTIVE_STATE GPIO_PIN_RESET

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* ESP8266 调试状态：在 VS Code Watch 中观察这些变量，判断 AT / WiFi / HTTP 步骤是否成功。 */
volatile HAL_StatusTypeDef esp_init_status;
volatile ESP8266_Result esp_check_result;
volatile ESP8266_Result esp_wifi_result;
volatile ESP8266_Result esp_http_result;
volatile ESP8266_Result esp_station_mode_result;
volatile WiFiMonitorState wifi_monitor_state;
volatile uint32_t wifi_connect_attempt_count;
volatile uint32_t wifi_connect_success_count;
volatile uint32_t wifi_connect_fail_count;
volatile uint32_t wifi_last_connect_tick;
volatile uint32_t gps_upload_success_count;
volatile uint32_t gps_upload_fail_count;
volatile uint32_t gps_upload_attempt_count;
volatile uint32_t gps_upload_skip_no_wifi_count;
volatile uint32_t gps_upload_skip_invalid_count;
volatile uint32_t distance_upload_skip_no_data_count;
volatile uint32_t debug_reset_flags;
volatile uint8_t debug_reset_low_power;
volatile uint8_t debug_reset_window_watchdog;
volatile uint8_t debug_reset_independent_watchdog;
volatile uint8_t debug_reset_software;
volatile uint8_t debug_reset_power;
volatile uint8_t debug_reset_pin;
volatile uint8_t debug_gps_valid;
volatile char debug_gps_utc_time[11];
volatile int32_t debug_gps_latitude_microdegrees;
volatile int32_t debug_gps_longitude_microdegrees;
volatile uint8_t debug_gps_satellites;
volatile int32_t debug_gps_altitude_meters;
volatile uint32_t debug_gps_last_parse_tick;
volatile uint32_t debug_ultrasonic_echo_us;
volatile uint32_t debug_ultrasonic_distance_mm;
volatile uint32_t debug_ultrasonic_capture_count;
volatile uint32_t debug_ultrasonic_timeout_count;
volatile uint8_t debug_ultrasonic_ready;
volatile uint8_t debug_ultrasonic_waiting_echo;
volatile HAL_StatusTypeDef debug_mpu6050_init_status;
volatile HAL_StatusTypeDef debug_mpu6050_read_status;
volatile uint8_t debug_mpu6050_who_am_i;
volatile uint8_t debug_mpu6050_ready;
volatile uint8_t debug_mpu6050_alarm;
volatile uint8_t debug_mpu6050_tilt_detected;
volatile uint8_t debug_mpu6050_motion_detected;
volatile int16_t debug_mpu6050_accel_x;
volatile int16_t debug_mpu6050_accel_y;
volatile int16_t debug_mpu6050_accel_z;
volatile int16_t debug_mpu6050_gyro_x;
volatile int16_t debug_mpu6050_gyro_y;
volatile int16_t debug_mpu6050_gyro_z;
volatile int32_t debug_mpu6050_tilt_x_mdeg;
volatile int32_t debug_mpu6050_tilt_y_mdeg;
volatile uint32_t debug_mpu6050_motion_raw;
volatile HAL_StatusTypeDef debug_mq2_adc_status;
volatile GPIO_PinState debug_mq2_do_state;
volatile uint32_t debug_mq2_ao_raw;
volatile uint32_t debug_mq2_concentration_permille;
volatile uint8_t debug_mq2_do_alarm;
volatile uint8_t debug_mq2_ao_alarm;
volatile uint8_t debug_mq2_alarm;
volatile uint32_t debug_mq2_sample_count;
static GPS_Fix current_gps_fix;
static uint32_t last_gps_upload_tick;
static uint32_t ultrasonic_rise_capture_us;
static uint32_t ultrasonic_last_trigger_tick;
static uint8_t ultrasonic_capture_falling_edge;
static uint32_t mpu6050_last_sample_tick;
static uint32_t mpu6050_last_init_attempt_tick;
static int16_t mpu6050_last_accel_x;
static int16_t mpu6050_last_accel_y;
static int16_t mpu6050_last_accel_z;
static uint8_t mpu6050_has_previous_sample;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void WiFi_ConnectOnce(void);
static void GPS_UpdateDebugFix(const GPS_Fix *fix);
static void DWT_DelayUs(uint32_t us);
static void Ultrasonic_Init(void);
static void Ultrasonic_Trigger(void);
static void Ultrasonic_CheckTimeout(void);
static int32_t Abs32(int32_t value);
static int32_t Clamp32(int32_t value, int32_t min_value, int32_t max_value);
static int16_t MPU6050_ReadInt16(const uint8_t *data);
static int32_t MPU6050_AccelToTiltMdeg(int16_t accel_raw);
static HAL_StatusTypeDef ADC_SelectChannel(uint32_t channel);
static void AlarmLed_Set(uint8_t active);
static void AlarmLed_SelfTest(void);
static void Led_UpdateAlarm(void);
static void MPU6050_Init(void);
static void MPU6050_TryInit(void);
static void MPU6050_Process(void);
static void MQ2_Process(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void WiFi_ConnectOnce(void)
{
  wifi_connect_attempt_count++;
  wifi_last_connect_tick = HAL_GetTick();

  if (WIFI_SSID[0] == '\0')
  {
    wifi_monitor_state = WIFI_MONITOR_FAILED;
    wifi_connect_fail_count++;
    esp_wifi_result = ESP8266_ERROR;
    return;
  }

  wifi_monitor_state = WIFI_MONITOR_CHECK_MODULE;
  esp_check_result = ESP8266_Check(2000);
  if (esp_check_result != ESP8266_OK)
  {
    wifi_monitor_state = WIFI_MONITOR_FAILED;
    wifi_connect_fail_count++;
    esp_wifi_result = esp_check_result;
    return;
  }

  wifi_monitor_state = WIFI_MONITOR_SET_STATION;
  esp_station_mode_result = ESP8266_SetStationMode(2000);
  if (esp_station_mode_result != ESP8266_OK)
  {
    wifi_monitor_state = WIFI_MONITOR_FAILED;
    wifi_connect_fail_count++;
    esp_wifi_result = esp_station_mode_result;
    return;
  }

  wifi_monitor_state = WIFI_MONITOR_CONNECTING;
  esp_wifi_result = ESP8266_ConnectWiFi(WIFI_SSID, WIFI_PASSWORD, 15000);
  if (esp_wifi_result == ESP8266_OK)
  {
    wifi_monitor_state = WIFI_MONITOR_CONNECTED;
    wifi_connect_success_count++;
    last_gps_upload_tick = HAL_GetTick();
  }
  else
  {
    wifi_monitor_state = WIFI_MONITOR_FAILED;
    wifi_connect_fail_count++;
  }
}

static void GPS_UpdateDebugFix(const GPS_Fix *fix)
{
  uint8_t i;

  if (fix == NULL)
  {
    return;
  }

  debug_gps_valid = fix->valid;
  debug_gps_latitude_microdegrees = fix->latitude_microdegrees;
  debug_gps_longitude_microdegrees = fix->longitude_microdegrees;
  debug_gps_satellites = fix->satellites;
  debug_gps_altitude_meters = fix->altitude_meters;
  debug_gps_last_parse_tick = HAL_GetTick();

  for (i = 0; i < sizeof(debug_gps_utc_time); i++)
  {
    debug_gps_utc_time[i] = fix->utc_time[i];
    if (fix->utc_time[i] == '\0')
    {
      break;
    }
  }

  if (i >= sizeof(debug_gps_utc_time))
  {
    debug_gps_utc_time[sizeof(debug_gps_utc_time) - 1U] = '\0';
  }
}

static void DWT_DelayUs(uint32_t us)
{
  uint32_t start_tick = DWT->CYCCNT;
  uint32_t delay_ticks = us * (HAL_RCC_GetHCLKFreq() / 1000000U);

  while ((DWT->CYCCNT - start_tick) < delay_ticks)
  {
  }
}

static void Ultrasonic_Init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  __HAL_TIM_SET_CAPTUREPOLARITY(&htim2, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_RISING);
  __HAL_TIM_SET_COUNTER(&htim2, 0);

  if (HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void Ultrasonic_Trigger(void)
{
  if (debug_ultrasonic_waiting_echo != 0U)
  {
    return;
  }

  debug_ultrasonic_waiting_echo = 1U;
  ultrasonic_capture_falling_edge = 0U;
  ultrasonic_last_trigger_tick = HAL_GetTick();

  __HAL_TIM_SET_COUNTER(&htim2, 0);
  __HAL_TIM_SET_CAPTUREPOLARITY(&htim2, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_RISING);

  HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_SET);
  DWT_DelayUs(10U);
  HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET);
}

static void Ultrasonic_CheckTimeout(void)
{
  if ((debug_ultrasonic_waiting_echo != 0U) &&
      ((HAL_GetTick() - ultrasonic_last_trigger_tick) >= ULTRASONIC_ECHO_TIMEOUT_MS))
  {
    debug_ultrasonic_waiting_echo = 0U;
    ultrasonic_capture_falling_edge = 0U;
    debug_ultrasonic_timeout_count++;
    __HAL_TIM_SET_CAPTUREPOLARITY(&htim2, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_RISING);
  }
}

static int32_t Abs32(int32_t value)
{
  return (value < 0) ? -value : value;
}

static int32_t Clamp32(int32_t value, int32_t min_value, int32_t max_value)
{
  if (value < min_value)
  {
    return min_value;
  }

  if (value > max_value)
  {
    return max_value;
  }

  return value;
}

static int16_t MPU6050_ReadInt16(const uint8_t *data)
{
  return (int16_t)(((uint16_t)data[0] << 8) | data[1]);
}

static int32_t MPU6050_AccelToTiltMdeg(int16_t accel_raw)
{
  int32_t tilt_mdeg = ((int32_t)accel_raw * MPU6050_MAX_TILT_MDEG) / MPU6050_ONE_G_RAW;

  return Clamp32(tilt_mdeg, -MPU6050_MAX_TILT_MDEG, MPU6050_MAX_TILT_MDEG);
}

static HAL_StatusTypeDef ADC_SelectChannel(uint32_t channel)
{
  ADC_ChannelConfTypeDef config = {0};

  config.Channel = channel;
  config.Rank = ADC_REGULAR_RANK_1;
  config.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;

  return HAL_ADC_ConfigChannel(&hadc1, &config);
}

static void AlarmLed_Set(uint8_t active)
{
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,
                    (active != 0U) ? ALARM_LED_ACTIVE_STATE : ALARM_LED_INACTIVE_STATE);
}

static void AlarmLed_SelfTest(void)
{
  AlarmLed_Set(1U);
  HAL_Delay(120U);
  AlarmLed_Set(0U);
  HAL_Delay(120U);
}

static void Led_UpdateAlarm(void)
{
  AlarmLed_Set(((debug_mpu6050_alarm != 0U) || (debug_mq2_alarm != 0U)) ? 1U : 0U);
}

static void MQ2_Process(void)
{
  static uint32_t mq2_last_sample_tick;
  GPIO_PinState do_state;

  if ((debug_mq2_sample_count != 0U) &&
      ((HAL_GetTick() - mq2_last_sample_tick) < MQ2_SAMPLE_INTERVAL_MS))
  {
    return;
  }
  mq2_last_sample_tick = HAL_GetTick();

  do_state = HAL_GPIO_ReadPin(MQ2_DO_GPIO_Port, MQ2_DO_Pin);
  debug_mq2_do_state = do_state;
  debug_mq2_do_alarm = (do_state == MQ2_DO_ALARM_STATE) ? 1U : 0U;

  (void)HAL_ADC_Stop(&hadc1);
  debug_mq2_adc_status = ADC_SelectChannel(MQ2_AO_ADC_CHANNEL);
  if (debug_mq2_adc_status == HAL_OK)
  {
    debug_mq2_adc_status = HAL_ADC_Start(&hadc1);
  }

  if (debug_mq2_adc_status == HAL_OK)
  {
    debug_mq2_adc_status = HAL_ADC_PollForConversion(&hadc1, 20U);
  }

  if (debug_mq2_adc_status == HAL_OK)
  {
    debug_mq2_ao_raw = HAL_ADC_GetValue(&hadc1);
    debug_mq2_concentration_permille = (debug_mq2_ao_raw * 1000U) / 4095U;
    debug_mq2_ao_alarm = (debug_mq2_ao_raw >= MQ2_AO_ALARM_THRESHOLD) ? 1U : 0U;
    debug_mq2_sample_count++;
  }

  (void)HAL_ADC_Stop(&hadc1);
  (void)ADC_SelectChannel(MQ2_DEFAULT_ADC_CHANNEL);

  debug_mq2_alarm = ((debug_mq2_do_alarm != 0U) || (debug_mq2_ao_alarm != 0U)) ? 1U : 0U;
  Led_UpdateAlarm();
}

static void MPU6050_Init(void)
{
  uint8_t pwr_mgmt = 0x00U;

  mpu6050_last_init_attempt_tick = HAL_GetTick();
  debug_mpu6050_ready = 0U;
  debug_mpu6050_alarm = 0U;
  debug_mpu6050_tilt_detected = 0U;
  debug_mpu6050_motion_detected = 0U;
  debug_mpu6050_tilt_x_mdeg = 0;
  debug_mpu6050_tilt_y_mdeg = 0;
  debug_mpu6050_motion_raw = 0U;
  Led_UpdateAlarm();

  debug_mpu6050_init_status = HAL_I2C_IsDeviceReady(&hi2c1, MPU6050_ADDR, 3U, 100U);
  if (debug_mpu6050_init_status != HAL_OK)
  {
    return;
  }

  debug_mpu6050_init_status = HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR,
                                               MPU6050_REG_WHO_AM_I,
                                               I2C_MEMADD_SIZE_8BIT,
                                               (uint8_t *)&debug_mpu6050_who_am_i,
                                               1U, 100U);
  if (debug_mpu6050_init_status != HAL_OK)
  {
    return;
  }

  debug_mpu6050_init_status = HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR,
                                                MPU6050_REG_PWR_MGMT_1,
                                                I2C_MEMADD_SIZE_8BIT,
                                                &pwr_mgmt, 1U, 100U);
  if (debug_mpu6050_init_status != HAL_OK)
  {
    return;
  }

  debug_mpu6050_ready = 1U;
  mpu6050_has_previous_sample = 0U;
  mpu6050_last_sample_tick = HAL_GetTick() - MPU6050_SAMPLE_INTERVAL_MS;
}

static void MPU6050_TryInit(void)
{
  if (debug_mpu6050_ready != 0U)
  {
    return;
  }

  if (HAL_GetTick() < MPU6050_POWER_ON_DELAY_MS)
  {
    return;
  }

  if ((HAL_GetTick() - mpu6050_last_init_attempt_tick) < MPU6050_INIT_RETRY_INTERVAL_MS)
  {
    return;
  }

  MPU6050_Init();
}

static void MPU6050_Process(void)
{
  uint8_t data[14];
  int32_t accel_delta;
  uint8_t tilt_detected;
  uint8_t motion_detected;

  if (debug_mpu6050_ready == 0U)
  {
    MPU6050_TryInit();
    debug_mpu6050_alarm = 0U;
    Led_UpdateAlarm();
    return;
  }

  if ((HAL_GetTick() - mpu6050_last_sample_tick) < MPU6050_SAMPLE_INTERVAL_MS)
  {
    return;
  }
  mpu6050_last_sample_tick = HAL_GetTick();

  debug_mpu6050_read_status = HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR,
                                               MPU6050_REG_ACCEL_XOUT_H,
                                               I2C_MEMADD_SIZE_8BIT,
                                               data, sizeof(data), 100U);
  if (debug_mpu6050_read_status != HAL_OK)
  {
    debug_mpu6050_alarm = 0U;
    Led_UpdateAlarm();
    return;
  }

  debug_mpu6050_accel_x = MPU6050_ReadInt16(&data[0]);
  debug_mpu6050_accel_y = MPU6050_ReadInt16(&data[2]);
  debug_mpu6050_accel_z = MPU6050_ReadInt16(&data[4]);
  debug_mpu6050_gyro_x = MPU6050_ReadInt16(&data[8]);
  debug_mpu6050_gyro_y = MPU6050_ReadInt16(&data[10]);
  debug_mpu6050_gyro_z = MPU6050_ReadInt16(&data[12]);
  debug_mpu6050_tilt_x_mdeg = MPU6050_AccelToTiltMdeg(debug_mpu6050_accel_x);
  debug_mpu6050_tilt_y_mdeg = MPU6050_AccelToTiltMdeg(debug_mpu6050_accel_y);

  tilt_detected =
      (Abs32(debug_mpu6050_accel_x) > MPU6050_TILT_THRESHOLD_RAW) ||
      (Abs32(debug_mpu6050_accel_y) > MPU6050_TILT_THRESHOLD_RAW);

  accel_delta = 0;
  if (mpu6050_has_previous_sample != 0U)
  {
    accel_delta += Abs32((int32_t)debug_mpu6050_accel_x - mpu6050_last_accel_x);
    accel_delta += Abs32((int32_t)debug_mpu6050_accel_y - mpu6050_last_accel_y);
    accel_delta += Abs32((int32_t)debug_mpu6050_accel_z - mpu6050_last_accel_z);
  }

  motion_detected =
      (accel_delta > MPU6050_ACCEL_MOVE_DELTA_RAW) ||
      (Abs32(debug_mpu6050_gyro_x) > MPU6050_GYRO_MOVE_THRESHOLD_RAW) ||
      (Abs32(debug_mpu6050_gyro_y) > MPU6050_GYRO_MOVE_THRESHOLD_RAW) ||
      (Abs32(debug_mpu6050_gyro_z) > MPU6050_GYRO_MOVE_THRESHOLD_RAW);
  debug_mpu6050_motion_raw =
      (uint32_t)accel_delta +
      (uint32_t)Abs32(debug_mpu6050_gyro_x) +
      (uint32_t)Abs32(debug_mpu6050_gyro_y) +
      (uint32_t)Abs32(debug_mpu6050_gyro_z);

  mpu6050_last_accel_x = debug_mpu6050_accel_x;
  mpu6050_last_accel_y = debug_mpu6050_accel_y;
  mpu6050_last_accel_z = debug_mpu6050_accel_z;
  mpu6050_has_previous_sample = 1U;

  debug_mpu6050_tilt_detected = tilt_detected;
  debug_mpu6050_motion_detected = motion_detected;
  debug_mpu6050_alarm = (tilt_detected || motion_detected) ? 1U : 0U;

  Led_UpdateAlarm();
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  debug_reset_flags = RCC->CSR;
  debug_reset_low_power = __HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST) ? 1U : 0U;
  debug_reset_window_watchdog = __HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) ? 1U : 0U;
  debug_reset_independent_watchdog = __HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) ? 1U : 0U;
  debug_reset_software = __HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) ? 1U : 0U;
  debug_reset_power = __HAL_RCC_GET_FLAG(RCC_FLAG_PORRST) ? 1U : 0U;
  debug_reset_pin = __HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) ? 1U : 0U;
  __HAL_RCC_CLEAR_RESET_FLAGS();

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  AlarmLed_SelfTest();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  /* USART1 接 ATGM332D，GPS 模块会主动输出 NMEA，不需要先发送命令。 */
  GPS_Init(&huart1);
  Ultrasonic_Init();
  mpu6050_last_init_attempt_tick = HAL_GetTick();

  /* USART3 接 ESP8266，用 AT 指令检测模块并连接 WiFi。联网放到主循环里，避免挡住报警检测。 */
  esp_init_status = ESP8266_Init(&huart3);
  wifi_monitor_state = WIFI_MONITOR_FAILED;
  wifi_last_connect_tick = HAL_GetTick() - WIFI_RECONNECT_INTERVAL_MS;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (GPS_ProcessNewSentence(&current_gps_fix))
    {
      GPS_UpdateDebugFix(&current_gps_fix);
    }

    MPU6050_Process();
    MQ2_Process();

    Ultrasonic_CheckTimeout();
    if ((HAL_GetTick() - ultrasonic_last_trigger_tick) >= ULTRASONIC_TRIGGER_INTERVAL_MS)
    {
      Ultrasonic_Trigger();
    }

    if ((wifi_monitor_state != WIFI_MONITOR_CONNECTED) &&
        ((HAL_GetTick() - wifi_last_connect_tick) >= WIFI_RECONNECT_INTERVAL_MS))
    {
      WiFi_ConnectOnce();
    }

    if ((HAL_GetTick() - last_gps_upload_tick) >= GPS_UPLOAD_INTERVAL_MS)
    {
      last_gps_upload_tick = HAL_GetTick();

      if (esp_wifi_result != ESP8266_OK)
      {
        gps_upload_skip_no_wifi_count++;
      }
      else if (debug_ultrasonic_ready == 0U)
      {
        distance_upload_skip_no_data_count++;
      }
      else
      {
        gps_upload_attempt_count++;
        esp_http_result = ESP8266_UploadDistance(API_HOST, API_PATH, API_PORT,
                                                 debug_ultrasonic_distance_mm,
                                                 debug_ultrasonic_echo_us,
                                                 debug_mpu6050_tilt_x_mdeg,
                                                 debug_mpu6050_tilt_y_mdeg,
                                                 debug_mpu6050_motion_raw,
                                                 debug_mpu6050_gyro_x,
                                                 debug_mpu6050_gyro_y,
                                                 debug_mpu6050_gyro_z,
                                                 debug_mq2_ao_raw,
                                                 debug_mq2_concentration_permille,
                                                 debug_mq2_alarm,
                                                 debug_mq2_do_alarm,
                                                 debug_mq2_ao_alarm,
                                                 &gps_latest_fix, 15000);

        if (esp_http_result == ESP8266_OK)
        {
          gps_upload_success_count++;
        }
        else
        {
          gps_upload_fail_count++;
        }
      }
    }
    HAL_Delay(10);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  uint32_t capture_us;
  uint32_t echo_width_us;

  if ((htim->Instance != TIM2) || (htim->Channel != HAL_TIM_ACTIVE_CHANNEL_2))
  {
    return;
  }

  capture_us = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);

  if (ultrasonic_capture_falling_edge == 0U)
  {
    ultrasonic_rise_capture_us = capture_us;
    ultrasonic_capture_falling_edge = 1U;
    __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_FALLING);
  }
  else
  {
    if (capture_us >= ultrasonic_rise_capture_us)
    {
      echo_width_us = capture_us - ultrasonic_rise_capture_us;
    }
    else
    {
      echo_width_us = (ULTRASONIC_TIMER_PERIOD_US - ultrasonic_rise_capture_us) + capture_us;
    }

    debug_ultrasonic_echo_us = echo_width_us;
    debug_ultrasonic_distance_mm = (echo_width_us * 343U) / 2000U;
    debug_ultrasonic_capture_count++;
    debug_ultrasonic_ready = 1U;
    debug_ultrasonic_waiting_echo = 0U;
    ultrasonic_capture_falling_edge = 0U;
    __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_RISING);
  }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
