#ifndef __ESP8266_H__
#define __ESP8266_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "gps.h"

#define ESP8266_RX_BUFFER_SIZE 512

typedef enum
{
  ESP8266_OK = 0,
  ESP8266_ERROR,
  ESP8266_TIMEOUT,
  ESP8266_BUSY
} ESP8266_Result;

extern volatile uint8_t esp8266_rx_buffer[ESP8266_RX_BUFFER_SIZE];
extern volatile uint16_t esp8266_rx_len;
extern volatile uint8_t esp8266_line_ready;
extern volatile uint8_t esp8266_overflow;
extern volatile HAL_StatusTypeDef esp8266_last_rx_status;
extern volatile HAL_StatusTypeDef esp8266_last_tx_status;

HAL_StatusTypeDef ESP8266_Init(UART_HandleTypeDef *huart);
void ESP8266_ResetBuffer(void);
ESP8266_Result ESP8266_Check(uint32_t timeout_ms);
ESP8266_Result ESP8266_SetStationMode(uint32_t timeout_ms);
ESP8266_Result ESP8266_ConnectWiFi(const char *ssid, const char *password, uint32_t timeout_ms);
ESP8266_Result ESP8266_HttpGet(const char *host, const char *path, uint16_t port, uint32_t timeout_ms);
ESP8266_Result ESP8266_UploadGpsFix(const char *host, const char *path, uint16_t port,
                                    const GPS_Fix *fix, uint32_t timeout_ms);
ESP8266_Result ESP8266_UploadDistance(const char *host, const char *path, uint16_t port,
                                      uint32_t distance_mm, uint32_t echo_us,
                                      int32_t tilt_x_mdeg, int32_t tilt_y_mdeg,
                                      uint32_t motion_raw,
                                      int16_t gyro_x_raw, int16_t gyro_y_raw, int16_t gyro_z_raw,
                                      uint32_t smoke_raw, uint32_t smoke_permille,
                                      uint8_t smoke_alarm, uint8_t smoke_do_alarm, uint8_t smoke_ao_alarm,
                                      const GPS_Fix *fix, uint32_t timeout_ms);
ESP8266_Result ESP8266_SendCommand(const char *cmd, const char *expected, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* __ESP8266_H__ */
