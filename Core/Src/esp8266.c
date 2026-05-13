#include "esp8266.h"
#include "gps.h"

#include <stdio.h>
#include <string.h>

volatile uint8_t esp8266_rx_buffer[ESP8266_RX_BUFFER_SIZE];
volatile uint16_t esp8266_rx_len;
volatile uint8_t esp8266_line_ready;
volatile uint8_t esp8266_overflow;
volatile HAL_StatusTypeDef esp8266_last_rx_status;
volatile HAL_StatusTypeDef esp8266_last_tx_status;

/* 当前 ESP8266 使用的串口句柄，本项目中传入的是 huart3。 */
static UART_HandleTypeDef *esp8266_uart;

/* ESP8266 串口每次中断接收 1 个字节。 */
static uint8_t esp8266_rx_byte;

/* 启动 ESP8266 串口接收中断，并保存 HAL 返回值方便调试。 */
static HAL_StatusTypeDef ESP8266_StartReceiveIT(void)
{
  if (esp8266_uart == NULL)
  {
    esp8266_last_rx_status = HAL_ERROR;
    return esp8266_last_rx_status;
  }

  esp8266_last_rx_status = HAL_UART_Receive_IT(esp8266_uart, &esp8266_rx_byte, 1);
  return esp8266_last_rx_status;
}

/* 直接向 ESP8266 发送原始字符串，例如 AT 指令或 HTTP 请求内容。 */
static ESP8266_Result ESP8266_SendRaw(const char *data, uint32_t timeout_ms)
{
  if ((esp8266_uart == NULL) || (data == NULL))
  {
    return ESP8266_ERROR;
  }

  esp8266_last_tx_status =
      HAL_UART_Transmit(esp8266_uart, (uint8_t *)data, strlen(data), timeout_ms);

  return (esp8266_last_tx_status == HAL_OK) ? ESP8266_OK : ESP8266_ERROR;
}

/* 在当前接收缓冲区中查找指定关键字，例如 OK、CONNECT、CLOSED。 */
static uint8_t ESP8266_BufferContains(const char *text)
{
  if (text == NULL)
  {
    return 0;
  }

  return strstr((const char *)esp8266_rx_buffer, text) != NULL;
}

/* 等待 ESP8266 返回指定关键字；如果先收到 ERROR/FAIL，则认为失败。 */
static ESP8266_Result ESP8266_WaitFor(const char *expected, uint32_t timeout_ms)
{
  uint32_t start_tick = HAL_GetTick();

  while ((HAL_GetTick() - start_tick) < timeout_ms)
  {
    if (ESP8266_BufferContains(expected))
    {
      return ESP8266_OK;
    }

    if (ESP8266_BufferContains("ERROR") || ESP8266_BufferContains("FAIL"))
    {
      return ESP8266_ERROR;
    }
  }

  return ESP8266_TIMEOUT;
}

/* 初始化 ESP8266 驱动：绑定串口、清缓冲区、启动接收中断。 */
HAL_StatusTypeDef ESP8266_Init(UART_HandleTypeDef *huart)
{
  esp8266_uart = huart;
  ESP8266_ResetBuffer();
  return ESP8266_StartReceiveIT();
}

/* 清空 ESP8266 接收缓冲区；发送新 AT 指令前通常先清一次。 */
void ESP8266_ResetBuffer(void)
{
  __disable_irq();
  memset((void *)esp8266_rx_buffer, 0, ESP8266_RX_BUFFER_SIZE);
  esp8266_rx_len = 0;
  esp8266_line_ready = 0;
  esp8266_overflow = 0;
  __enable_irq();
}

/* 发送一条 AT 指令，并等待指定关键字。 */
ESP8266_Result ESP8266_SendCommand(const char *cmd, const char *expected, uint32_t timeout_ms)
{
  ESP8266_ResetBuffer();

  if (ESP8266_SendRaw(cmd, timeout_ms) != ESP8266_OK)
  {
    return ESP8266_ERROR;
  }

  return ESP8266_WaitFor(expected, timeout_ms);
}

/* 基础连通性检测：ESP8266 正常时会返回 OK。 */
ESP8266_Result ESP8266_Check(uint32_t timeout_ms)
{
  return ESP8266_SendCommand("AT\r\n", "OK", timeout_ms);
}

/* 设置为 STA 模式，用于连接路由器热点。 */
ESP8266_Result ESP8266_SetStationMode(uint32_t timeout_ms)
{
  return ESP8266_SendCommand("AT+CWMODE=1\r\n", "OK", timeout_ms);
}

/* 连接 WiFi，成功后通常会看到 WIFI GOT IP。 */
ESP8266_Result ESP8266_ConnectWiFi(const char *ssid, const char *password, uint32_t timeout_ms)
{
  char cmd[160];

  snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
  return ESP8266_SendCommand(cmd, "WIFI GOT IP", timeout_ms);
}

/* 使用 ESP8266 建立 TCP 连接并发送一个简单的 HTTP GET 请求。 */
ESP8266_Result ESP8266_HttpGet(const char *host, const char *path, uint16_t port, uint32_t timeout_ms)
{
  char cmd[96];
  char host_header[96];
  char request[1024];
  ESP8266_Result result;

  snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%u\r\n", host, port);
  result = ESP8266_SendCommand(cmd, "CONNECT", timeout_ms);
  if (result != ESP8266_OK)
  {
    return result;
  }

  if (port == 80U)
  {
    snprintf(host_header, sizeof(host_header), "%s", host);
  }
  else
  {
    snprintf(host_header, sizeof(host_header), "%s:%u", host, port);
  }

  snprintf(request, sizeof(request),
           "GET %s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "Connection: close\r\n"
           "\r\n",
           path, host_header);

  snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u\r\n", (unsigned int)strlen(request));
  result = ESP8266_SendCommand(cmd, ">", timeout_ms);
  if (result != ESP8266_OK)
  {
    return result;
  }

  ESP8266_ResetBuffer();
  if (ESP8266_SendRaw(request, timeout_ms) != ESP8266_OK)
  {
    return ESP8266_ERROR;
  }

  return ESP8266_WaitFor("CLOSED", timeout_ms);
}

ESP8266_Result ESP8266_UploadGpsFix(const char *host, const char *path, uint16_t port,
                                    const GPS_Fix *fix, uint32_t timeout_ms)
{
  char request_path[192];
  char separator = '?';

  if ((fix == NULL) || !fix->valid)
  {
    return ESP8266_ERROR;
  }

  if (strchr(path, '?') != NULL)
  {
    separator = '&';
  }

  snprintf(request_path, sizeof(request_path),
           "%s%cvalid=%u&lat=%ld&lon=%ld&sat=%u&alt=%ld&utc=%s",
           path,
           separator,
           fix->valid,
           (long)fix->latitude_microdegrees,
           (long)fix->longitude_microdegrees,
           fix->satellites,
           (long)fix->altitude_meters,
           fix->utc_time);

  return ESP8266_HttpGet(host, request_path, port, timeout_ms);
}

ESP8266_Result ESP8266_UploadDistance(const char *host, const char *path, uint16_t port,
                                      uint32_t distance_mm, uint32_t echo_us,
                                      int32_t tilt_x_mdeg, int32_t tilt_y_mdeg,
                                      uint32_t motion_raw,
                                      int16_t gyro_x_raw, int16_t gyro_y_raw, int16_t gyro_z_raw,
                                      uint32_t smoke_raw, uint32_t smoke_permille,
                                      uint8_t smoke_alarm, uint8_t smoke_do_alarm, uint8_t smoke_ao_alarm,
                                      const GPS_Fix *fix, uint32_t timeout_ms)
{
  char request_path[768];
  char device_id[25];
  char separator = '?';

  if (strchr(path, '?') != NULL)
  {
    separator = '&';
  }

  snprintf(device_id, sizeof(device_id),
           "%08lX%08lX%08lX",
           (unsigned long)HAL_GetUIDw0(),
           (unsigned long)HAL_GetUIDw1(),
           (unsigned long)HAL_GetUIDw2());

  if ((fix != NULL) && fix->valid)
  {
    snprintf(request_path, sizeof(request_path),
             "%s%cdevice_id=%s&distance_mm=%lu&echo_us=%lu"
             "&tilt_x_mdeg=%ld&tilt_y_mdeg=%ld&motion_raw=%lu"
             "&gyro_x_raw=%ld&gyro_y_raw=%ld&gyro_z_raw=%ld"
             "&smoke_raw=%lu&smoke_permille=%lu&smoke_alarm=%u"
             "&smoke_do_alarm=%u&smoke_ao_alarm=%u"
             "&valid=%u&lat=%ld&lon=%ld&sat=%u&alt=%ld&utc=%s",
             path,
             separator,
             device_id,
             (unsigned long)distance_mm,
             (unsigned long)echo_us,
             (long)tilt_x_mdeg,
             (long)tilt_y_mdeg,
             (unsigned long)motion_raw,
             (long)gyro_x_raw,
             (long)gyro_y_raw,
             (long)gyro_z_raw,
             (unsigned long)smoke_raw,
             (unsigned long)smoke_permille,
             smoke_alarm,
             smoke_do_alarm,
             smoke_ao_alarm,
             fix->valid,
             (long)fix->latitude_microdegrees,
             (long)fix->longitude_microdegrees,
             fix->satellites,
             (long)fix->altitude_meters,
             fix->utc_time);
  }
  else
  {
    snprintf(request_path, sizeof(request_path),
             "%s%cdevice_id=%s&distance_mm=%lu&echo_us=%lu"
             "&tilt_x_mdeg=%ld&tilt_y_mdeg=%ld&motion_raw=%lu"
             "&gyro_x_raw=%ld&gyro_y_raw=%ld&gyro_z_raw=%ld"
             "&smoke_raw=%lu&smoke_permille=%lu&smoke_alarm=%u"
             "&smoke_do_alarm=%u&smoke_ao_alarm=%u"
             "&valid=0",
             path,
             separator,
             device_id,
             (unsigned long)distance_mm,
             (unsigned long)echo_us,
             (long)tilt_x_mdeg,
             (long)tilt_y_mdeg,
             (unsigned long)motion_raw,
             (long)gyro_x_raw,
             (long)gyro_y_raw,
             (long)gyro_z_raw,
             (unsigned long)smoke_raw,
             (unsigned long)smoke_permille,
             smoke_alarm,
             smoke_do_alarm,
             smoke_ao_alarm);
  }

  return ESP8266_HttpGet(host, request_path, port, timeout_ms);
}

/*
 * HAL 全局串口接收完成回调。
 * 这里做分发：
 * - USART3：ESP8266 AT 指令回复
 * - USART1：ATGM332D GPS NMEA 数据
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if ((esp8266_uart != NULL) && (huart->Instance == esp8266_uart->Instance))
  {
    if (esp8266_rx_len < (ESP8266_RX_BUFFER_SIZE - 1))
    {
      esp8266_rx_buffer[esp8266_rx_len++] = esp8266_rx_byte;
      esp8266_rx_buffer[esp8266_rx_len] = '\0';
    }
    else
    {
      esp8266_overflow = 1;
    }

    if ((esp8266_rx_byte == '\r') || (esp8266_rx_byte == '\n'))
    {
      esp8266_line_ready = 1;
    }

    ESP8266_StartReceiveIT();
  }
  else if (huart->Instance == USART1)
  {
    GPS_HandleRxCplt(huart);
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    GPS_HandleError(huart);
  }
}
