#ifndef __GPS_H__
#define __GPS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define GPS_SENTENCE_BUFFER_SIZE 128
#define GPS_RAW_DEBUG_BUFFER_SIZE 64
#define GPS_SENTENCE_HISTORY_COUNT 4
#define GPS_STREAM_DEBUG_BUFFER_SIZE 256

typedef struct
{
  uint8_t valid;
  char utc_time[11];
  int32_t latitude_microdegrees;
  int32_t longitude_microdegrees;
  uint8_t satellites;
  int32_t altitude_meters;
} GPS_Fix;

extern volatile char gps_current_sentence[GPS_SENTENCE_BUFFER_SIZE];
extern volatile char gps_last_sentence[GPS_SENTENCE_BUFFER_SIZE];
extern volatile uint16_t gps_current_len;
extern volatile uint8_t gps_sentence_ready;
extern volatile uint8_t gps_overflow;
extern volatile HAL_StatusTypeDef gps_last_rx_status;
extern volatile uint32_t gps_rx_count;
extern volatile uint8_t gps_last_byte;
extern volatile uint32_t gps_line_end_count;
extern volatile uint8_t gps_raw_debug[GPS_RAW_DEBUG_BUFFER_SIZE];
extern volatile uint16_t gps_raw_debug_index;
extern volatile uint32_t gps_uart_error_code;
extern volatile uint32_t gps_uart_error_count;
extern volatile uint8_t gps_fix_ready;
extern volatile uint32_t gps_sentence_parse_count;
extern volatile uint32_t gps_valid_fix_count;
extern volatile char gps_sentence_history[GPS_SENTENCE_HISTORY_COUNT][GPS_SENTENCE_BUFFER_SIZE];
extern volatile uint8_t gps_sentence_history_index;
extern volatile uint32_t gps_sentence_history_count;
extern volatile char gps_debug_sentence_0[GPS_SENTENCE_BUFFER_SIZE];
extern volatile char gps_debug_sentence_1[GPS_SENTENCE_BUFFER_SIZE];
extern volatile char gps_debug_sentence_2[GPS_SENTENCE_BUFFER_SIZE];
extern volatile char gps_debug_sentence_3[GPS_SENTENCE_BUFFER_SIZE];
extern volatile char *gps_debug_sentence_0_ptr;
extern volatile char *gps_debug_sentence_1_ptr;
extern volatile char *gps_debug_sentence_2_ptr;
extern volatile char *gps_debug_sentence_3_ptr;
extern volatile char *gps_last_sentence_ptr;
extern volatile char gps_stream_debug[GPS_STREAM_DEBUG_BUFFER_SIZE];
extern volatile uint16_t gps_stream_debug_index;
extern volatile uint8_t gps_stream_debug_wrapped;
extern GPS_Fix gps_latest_fix;

HAL_StatusTypeDef GPS_Init(UART_HandleTypeDef *huart);
HAL_StatusTypeDef GPS_StartReceiveIT(void);
void GPS_OnByteReceived(uint8_t byte);
void GPS_HandleRxCplt(UART_HandleTypeDef *huart);
void GPS_HandleError(UART_HandleTypeDef *huart);
uint8_t GPS_ProcessNewSentence(GPS_Fix *fix);

#ifdef __cplusplus
}
#endif

#endif /* __GPS_H__ */
