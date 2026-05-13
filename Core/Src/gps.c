#include "gps.h"

#include <string.h>

/* USART1 每次中断接收 1 个字节。 */
static UART_HandleTypeDef *gps_uart;
static uint8_t gps_rx_byte;

/* 正在接收的一句 NMEA 数据，例如 $GNRMC,... */
volatile char gps_current_sentence[GPS_SENTENCE_BUFFER_SIZE];

/* 最近完整收到的一句 NMEA 数据，调试时主要看这个变量。 */
volatile char gps_last_sentence[GPS_SENTENCE_BUFFER_SIZE];
volatile uint16_t gps_current_len;

/* 收到完整一行后置 1；缓冲区写满后 gps_overflow 置 1。 */
volatile uint8_t gps_sentence_ready;
volatile uint8_t gps_overflow;

/* HAL_UART_Receive_IT 的返回值，0 表示 HAL_OK。 */
volatile HAL_StatusTypeDef gps_last_rx_status;

/* GPS 调试辅助变量：用来确认 USART1 是否真的收到过字节。 */
volatile uint32_t gps_rx_count;
volatile uint8_t gps_last_byte;
volatile uint32_t gps_line_end_count;
volatile uint8_t gps_raw_debug[GPS_RAW_DEBUG_BUFFER_SIZE];
volatile uint16_t gps_raw_debug_index;
volatile uint32_t gps_uart_error_code;
volatile uint32_t gps_uart_error_count;
volatile uint8_t gps_fix_ready;
volatile uint32_t gps_sentence_parse_count;
volatile uint32_t gps_valid_fix_count;
volatile char gps_sentence_history[GPS_SENTENCE_HISTORY_COUNT][GPS_SENTENCE_BUFFER_SIZE];
volatile uint8_t gps_sentence_history_index;
volatile uint32_t gps_sentence_history_count;
volatile char gps_debug_sentence_0[GPS_SENTENCE_BUFFER_SIZE];
volatile char gps_debug_sentence_1[GPS_SENTENCE_BUFFER_SIZE];
volatile char gps_debug_sentence_2[GPS_SENTENCE_BUFFER_SIZE];
volatile char gps_debug_sentence_3[GPS_SENTENCE_BUFFER_SIZE];
volatile char *gps_debug_sentence_0_ptr = (volatile char *)gps_debug_sentence_0;
volatile char *gps_debug_sentence_1_ptr = (volatile char *)gps_debug_sentence_1;
volatile char *gps_debug_sentence_2_ptr = (volatile char *)gps_debug_sentence_2;
volatile char *gps_debug_sentence_3_ptr = (volatile char *)gps_debug_sentence_3;
volatile char *gps_last_sentence_ptr = (volatile char *)gps_last_sentence;
volatile char gps_stream_debug[GPS_STREAM_DEBUG_BUFFER_SIZE];
volatile uint16_t gps_stream_debug_index;
volatile uint8_t gps_stream_debug_wrapped;
GPS_Fix gps_latest_fix;

static uint8_t GPS_CopyField(const char *sentence, uint8_t field_index, char *out, uint8_t out_size);
static uint8_t GPS_ParseCoordinate(const char *value, char direction, int32_t *microdegrees);
static int32_t GPS_ParseSignedInteger(const char *value);
static void GPS_CopyString(char *dst, uint8_t dst_size, const char *src);
static uint8_t GPS_ParseRmc(const char *sentence, GPS_Fix *fix);
static uint8_t GPS_ParseGga(const char *sentence, GPS_Fix *fix);
static void GPS_SaveSentenceToHistory(void);
static void GPS_CopySentence(volatile char *dst, const volatile char *src);

HAL_StatusTypeDef GPS_Init(UART_HandleTypeDef *huart)
{
  gps_uart = huart;
  gps_current_len = 0;
  gps_sentence_ready = 0;
  gps_overflow = 0;
  gps_rx_count = 0;
  gps_line_end_count = 0;
  gps_uart_error_code = 0;
  gps_uart_error_count = 0;
  gps_fix_ready = 0;
  gps_sentence_parse_count = 0;
  gps_valid_fix_count = 0;
  gps_sentence_history_index = 0;
  gps_sentence_history_count = 0;
  gps_stream_debug_index = 0;
  gps_stream_debug_wrapped = 0;
  memset((void *)gps_sentence_history, 0, sizeof(gps_sentence_history));
  memset((void *)gps_debug_sentence_0, 0, sizeof(gps_debug_sentence_0));
  memset((void *)gps_debug_sentence_1, 0, sizeof(gps_debug_sentence_1));
  memset((void *)gps_debug_sentence_2, 0, sizeof(gps_debug_sentence_2));
  memset((void *)gps_debug_sentence_3, 0, sizeof(gps_debug_sentence_3));
  memset((void *)gps_stream_debug, 0, sizeof(gps_stream_debug));
  memset(&gps_latest_fix, 0, sizeof(gps_latest_fix));

  return GPS_StartReceiveIT();
}

/* 启动 1 字节中断接收，用来接 ATGM332D 主动输出的 NMEA 数据。 */
HAL_StatusTypeDef GPS_StartReceiveIT(void)
{
  if (gps_uart == NULL)
  {
    gps_last_rx_status = HAL_ERROR;
    return gps_last_rx_status;
  }

  gps_last_rx_status = HAL_UART_Receive_IT(gps_uart, &gps_rx_byte, 1);

  return gps_last_rx_status;
}

/* 处理 GPS 收到的单个字节，并按 NMEA 的一行一行格式保存。 */
void GPS_OnByteReceived(uint8_t byte)
{
  gps_rx_count++;
  gps_last_byte = byte;
  gps_raw_debug[gps_raw_debug_index] = byte;
  gps_raw_debug_index = (gps_raw_debug_index + 1) % GPS_RAW_DEBUG_BUFFER_SIZE;
  gps_stream_debug[gps_stream_debug_index] = byte;
  gps_stream_debug_index++;
  if (gps_stream_debug_index >= GPS_STREAM_DEBUG_BUFFER_SIZE)
  {
    gps_stream_debug_index = 0;
    gps_stream_debug_wrapped = 1;
  }

  /* NMEA 每一句通常以 '$' 开头，收到它就重新开始收一行。 */
  if (byte == '$')
  {
    gps_current_len = 0;
    gps_current_sentence[gps_current_len++] = byte;
    gps_current_sentence[gps_current_len] = '\0';
    return;
  }

  /* 遇到回车或换行，认为当前 NMEA 句子结束。 */
  if ((byte == '\r') || (byte == '\n'))
  {
    gps_line_end_count++;

    if (gps_current_len > 0)
    {
      gps_current_sentence[gps_current_len] = '\0';

      for (uint16_t i = 0; i <= gps_current_len; i++)
      {
        gps_last_sentence[i] = gps_current_sentence[i];
      }
      GPS_SaveSentenceToHistory();

      /* 提醒主循环或调试器：gps_last_sentence 已经有新的一整句。 */
      gps_sentence_ready = 1;
      gps_current_len = 0;
    }

    return;
  }

  /* 普通字符继续追加到当前句子。 */
  if (gps_current_len < (GPS_SENTENCE_BUFFER_SIZE - 1))
  {
    gps_current_sentence[gps_current_len++] = byte;
    gps_current_sentence[gps_current_len] = '\0';
  }
  else
  {
    /* 缓冲区满了就丢弃当前句，避免数组越界。 */
    gps_overflow = 1;
    gps_current_len = 0;
  }
}

static void GPS_SaveSentenceToHistory(void)
{
  GPS_CopySentence(gps_sentence_history[gps_sentence_history_index], gps_last_sentence);

  if (gps_sentence_history_index == 0U)
  {
    GPS_CopySentence(gps_debug_sentence_0, gps_last_sentence);
  }
  else if (gps_sentence_history_index == 1U)
  {
    GPS_CopySentence(gps_debug_sentence_1, gps_last_sentence);
  }
  else if (gps_sentence_history_index == 2U)
  {
    GPS_CopySentence(gps_debug_sentence_2, gps_last_sentence);
  }
  else
  {
    GPS_CopySentence(gps_debug_sentence_3, gps_last_sentence);
  }

  gps_sentence_history_index++;
  if (gps_sentence_history_index >= GPS_SENTENCE_HISTORY_COUNT)
  {
    gps_sentence_history_index = 0;
  }

  gps_sentence_history_count++;
}

static void GPS_CopySentence(volatile char *dst, const volatile char *src)
{
  for (uint16_t i = 0; i < GPS_SENTENCE_BUFFER_SIZE; i++)
  {
    dst[i] = src[i];
    if (src[i] == '\0')
    {
      break;
    }
  }
}

void GPS_HandleRxCplt(UART_HandleTypeDef *huart)
{
  if ((gps_uart != NULL) && (huart->Instance == gps_uart->Instance))
  {
    GPS_OnByteReceived(gps_rx_byte);
    GPS_StartReceiveIT();
  }
}

void GPS_HandleError(UART_HandleTypeDef *huart)
{
  if ((gps_uart != NULL) && (huart->Instance == gps_uart->Instance))
  {
    gps_uart_error_code = huart->ErrorCode;
    gps_uart_error_count++;
    GPS_StartReceiveIT();
  }
}

uint8_t GPS_ProcessNewSentence(GPS_Fix *fix)
{
  char sentence[GPS_SENTENCE_BUFFER_SIZE];
  uint8_t parsed = 0;

  if (!gps_sentence_ready)
  {
    return 0;
  }

  __disable_irq();
  for (uint16_t i = 0; i < GPS_SENTENCE_BUFFER_SIZE; i++)
  {
    sentence[i] = gps_last_sentence[i];
    if (sentence[i] == '\0')
    {
      break;
    }
  }
  gps_sentence_ready = 0;
  __enable_irq();

  if ((strncmp(sentence, "$GNRMC", 6) == 0) || (strncmp(sentence, "$GPRMC", 6) == 0))
  {
    parsed = GPS_ParseRmc(sentence, &gps_latest_fix);
  }
  else if ((strncmp(sentence, "$GNGGA", 6) == 0) || (strncmp(sentence, "$GPGGA", 6) == 0))
  {
    parsed = GPS_ParseGga(sentence, &gps_latest_fix);
  }

  if (parsed)
  {
    gps_sentence_parse_count++;
    gps_fix_ready = gps_latest_fix.valid;

    if (gps_latest_fix.valid)
    {
      gps_valid_fix_count++;
    }

    if (fix != NULL)
    {
      *fix = gps_latest_fix;
    }
  }

  return parsed;
}

static uint8_t GPS_CopyField(const char *sentence, uint8_t field_index, char *out, uint8_t out_size)
{
  uint8_t current_field = 0;
  uint8_t out_index = 0;

  if ((sentence == NULL) || (out == NULL) || (out_size == 0))
  {
    return 0;
  }

  out[0] = '\0';

  while ((*sentence != '\0') && (*sentence != '*'))
  {
    if (*sentence == ',')
    {
      current_field++;
      sentence++;
      continue;
    }

    if (current_field == field_index)
    {
      if (out_index < (out_size - 1))
      {
        out[out_index++] = *sentence;
      }
    }

    sentence++;
  }

  out[out_index] = '\0';
  return out_index > 0;
}

static uint8_t GPS_ParseCoordinate(const char *value, char direction, int32_t *microdegrees)
{
  uint32_t int_part = 0;
  uint32_t frac_part = 0;
  uint32_t frac_scale = 1;
  uint32_t degrees;
  uint32_t minutes;
  uint32_t minute_microdegrees;

  if ((value == NULL) || (microdegrees == NULL) || (value[0] == '\0'))
  {
    return 0;
  }

  while ((*value >= '0') && (*value <= '9'))
  {
    int_part = (int_part * 10U) + (uint32_t)(*value - '0');
    value++;
  }

  if (*value == '.')
  {
    value++;
    while ((*value >= '0') && (*value <= '9') && (frac_scale < 1000000U))
    {
      frac_part = (frac_part * 10U) + (uint32_t)(*value - '0');
      frac_scale *= 10U;
      value++;
    }
  }

  degrees = int_part / 100U;
  minutes = int_part % 100U;
  minute_microdegrees = (minutes * 1000000U) + ((frac_part * 1000000U) / frac_scale);
  *microdegrees = (int32_t)((degrees * 1000000U) + (minute_microdegrees / 60U));

  if ((direction == 'S') || (direction == 'W'))
  {
    *microdegrees = -*microdegrees;
  }

  return 1;
}

static int32_t GPS_ParseSignedInteger(const char *value)
{
  int32_t result = 0;
  int8_t sign = 1;

  if (value == NULL)
  {
    return 0;
  }

  if (*value == '-')
  {
    sign = -1;
    value++;
  }

  while ((*value >= '0') && (*value <= '9'))
  {
    result = (result * 10) + (*value - '0');
    value++;
  }

  return result * sign;
}

static void GPS_CopyString(char *dst, uint8_t dst_size, const char *src)
{
  uint8_t i = 0;

  if ((dst == NULL) || (src == NULL) || (dst_size == 0))
  {
    return;
  }

  while ((src[i] != '\0') && (i < (dst_size - 1)))
  {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
}

static uint8_t GPS_ParseRmc(const char *sentence, GPS_Fix *fix)
{
  char time_field[16];
  char status_field[4];
  char lat_field[16];
  char ns_field[4];
  char lon_field[16];
  char ew_field[4];
  int32_t latitude;
  int32_t longitude;

  if (fix == NULL)
  {
    return 0;
  }

  if (!GPS_CopyField(sentence, 1, time_field, sizeof(time_field)) ||
      !GPS_CopyField(sentence, 2, status_field, sizeof(status_field)) ||
      !GPS_CopyField(sentence, 3, lat_field, sizeof(lat_field)) ||
      !GPS_CopyField(sentence, 4, ns_field, sizeof(ns_field)) ||
      !GPS_CopyField(sentence, 5, lon_field, sizeof(lon_field)) ||
      !GPS_CopyField(sentence, 6, ew_field, sizeof(ew_field)))
  {
    fix->valid = 0;
    return 1;
  }

  GPS_CopyString(fix->utc_time, sizeof(fix->utc_time), time_field);
  fix->valid = status_field[0] == 'A';

  if (fix->valid &&
      GPS_ParseCoordinate(lat_field, ns_field[0], &latitude) &&
      GPS_ParseCoordinate(lon_field, ew_field[0], &longitude))
  {
    fix->latitude_microdegrees = latitude;
    fix->longitude_microdegrees = longitude;
  }

  return 1;
}

static uint8_t GPS_ParseGga(const char *sentence, GPS_Fix *fix)
{
  char time_field[16];
  char lat_field[16];
  char ns_field[4];
  char lon_field[16];
  char ew_field[4];
  char quality_field[4];
  char satellites_field[4];
  char altitude_field[12];
  int32_t latitude;
  int32_t longitude;

  if (fix == NULL)
  {
    return 0;
  }

  if (!GPS_CopyField(sentence, 1, time_field, sizeof(time_field)) ||
      !GPS_CopyField(sentence, 2, lat_field, sizeof(lat_field)) ||
      !GPS_CopyField(sentence, 3, ns_field, sizeof(ns_field)) ||
      !GPS_CopyField(sentence, 4, lon_field, sizeof(lon_field)) ||
      !GPS_CopyField(sentence, 5, ew_field, sizeof(ew_field)) ||
      !GPS_CopyField(sentence, 6, quality_field, sizeof(quality_field)))
  {
    fix->valid = 0;
    return 1;
  }

  GPS_CopyString(fix->utc_time, sizeof(fix->utc_time), time_field);
  fix->valid = quality_field[0] != '0';

  if (GPS_CopyField(sentence, 7, satellites_field, sizeof(satellites_field)))
  {
    fix->satellites = (uint8_t)GPS_ParseSignedInteger(satellites_field);
  }

  if (GPS_CopyField(sentence, 9, altitude_field, sizeof(altitude_field)))
  {
    fix->altitude_meters = GPS_ParseSignedInteger(altitude_field);
  }

  if (fix->valid &&
      GPS_ParseCoordinate(lat_field, ns_field[0], &latitude) &&
      GPS_ParseCoordinate(lon_field, ew_field[0], &longitude))
  {
    fix->latitude_microdegrees = latitude;
    fix->longitude_microdegrees = longitude;
  }

  return 1;
}
