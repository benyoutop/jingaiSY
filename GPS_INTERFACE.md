# GPS 接口文档

本文档描述当前固件中 GPS 模块的硬件连接、串口参数、软件接口、解析结果和调试变量。

## 硬件连接

当前工程使用 `USART1` 接 GPS 模块，并启用了 USART1 重映射：

| STM32 引脚 | 功能 | 连接 GPS 模块 |
| --- | --- | --- |
| `PB6` | `USART1_TX` | GPS RX，可选 |
| `PB7` | `USART1_RX` | GPS TX，必接 |
| `GND` | 地 | GPS GND |
| 电源 | 模块供电 | 按 GPS 模块规格连接 |

注意：`GPS_TX_Pin` 在代码里表示 STM32 的 TX 引脚，也就是 `PB6`。GPS 模块自己的 TX 应接到 STM32 的 `PB7 / USART1_RX`。

## 串口参数

配置位置：`Core/Src/usart.c`

```c
huart1.Instance = USART1;
huart1.Init.BaudRate = 9600;
huart1.Init.WordLength = UART_WORDLENGTH_8B;
huart1.Init.StopBits = UART_STOPBITS_1;
huart1.Init.Parity = UART_PARITY_NONE;
huart1.Init.Mode = UART_MODE_TX_RX;
huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
```

接收方式：

- 使用 `HAL_UART_Receive_IT()` 每次接收 1 字节。
- 在 `HAL_UART_RxCpltCallback()` 中分发到 `GPS_HandleRxCplt()`。
- 收到 `$` 后开始记录一条 NMEA 句子。
- 收到 `\r` 或 `\n` 后认为当前句子结束。

## 初始化流程

主程序初始化位置：`Core/Src/main.c`

```c
MX_USART1_UART_Init();
GPS_Init(&huart1);
```

`GPS_Init()` 会：

1. 保存 UART 句柄。
2. 清空 GPS 调试计数和缓冲区。
3. 清空 `gps_latest_fix`。
4. 启动 1 字节中断接收。

## 数据结构

定义位置：`Core/Inc/gps.h`

```c
typedef struct
{
  uint8_t valid;
  char utc_time[11];
  int32_t latitude_microdegrees;
  int32_t longitude_microdegrees;
  uint8_t satellites;
  int32_t altitude_meters;
} GPS_Fix;
```

字段说明：

| 字段 | 说明 |
| --- | --- |
| `valid` | 定位是否有效。`1` 有效，`0` 无效。 |
| `utc_time` | UTC 时间，来自 NMEA 原始字段，通常为 `hhmmss.sss`。 |
| `latitude_microdegrees` | 纬度，单位为微度。真实纬度 = 该值 / 1000000。北纬为正，南纬为负。 |
| `longitude_microdegrees` | 经度，单位为微度。真实经度 = 该值 / 1000000。东经为正，西经为负。 |
| `satellites` | 卫星数量，来自 GGA 句子。 |
| `altitude_meters` | 海拔高度，单位米，目前只保留整数部分。 |

## 对外函数

### `HAL_StatusTypeDef GPS_Init(UART_HandleTypeDef *huart)`

初始化 GPS 驱动并启动串口中断接收。

### `HAL_StatusTypeDef GPS_StartReceiveIT(void)`

启动下一次 1 字节中断接收。通常由 `GPS_Init()`、`GPS_HandleRxCplt()` 和 `GPS_HandleError()` 内部调用。

### `void GPS_HandleRxCplt(UART_HandleTypeDef *huart)`

GPS 串口接收完成回调入口。由全局 `HAL_UART_RxCpltCallback()` 调用。

### `void GPS_HandleError(UART_HandleTypeDef *huart)`

GPS 串口错误回调入口。会记录错误码并重新启动接收。

### `uint8_t GPS_ProcessNewSentence(GPS_Fix *fix)`

在主循环中调用，用于解析新收到的完整 NMEA 句子。

返回值：

| 返回值 | 说明 |
| --- | --- |
| `0` | 没有新句子，或新句子不是当前支持解析的类型。 |
| `1` | 成功解析了一条 RMC 或 GGA 句子。定位是否有效需继续看 `fix->valid` 或 `gps_latest_fix.valid`。 |

典型用法：

```c
if (GPS_ProcessNewSentence(&current_gps_fix))
{
  if (current_gps_fix.valid)
  {
    /* 可以使用经纬度 */
  }
}
```

## 当前支持解析的 NMEA 句子

当前只解析：

| 句子 | 用途 | 有效性判断 |
| --- | --- | --- |
| `$GNRMC` | 推荐最小定位信息，多星座组合 | 状态字段为 `A` |
| `$GPRMC` | GPS 推荐最小定位信息 | 状态字段为 `A` |
| `$GNGGA` | 固定定位数据，多星座组合 | 定位质量字段不是 `0` |
| `$GPGGA` | GPS 固定定位数据 | 定位质量字段不是 `0` |

当前会接收但不会解析为定位结果的常见句子：

| 句子 | 含义 |
| --- | --- |
| `$GPTXT` | 模块文本/状态信息，没有经纬度。 |
| `$GPGSA` | GPS DOP 和参与定位卫星信息。第三字段 `1/2/3` 表示无定位/2D/3D。 |
| `$BDGSA` | 北斗 DOP 和参与定位卫星信息。第三字段 `1/2/3` 表示无定位/2D/3D。 |
| `$GPGSV` / `$BDGSV` | 可见卫星信息，没有最终经纬度。 |

如果看到 `$GPGSA,A,1,...` 或 `$BDGSA,A,1,...`，说明模块正在输出数据，但还没有定位成功。

## 调试变量

在 VS Code Watch 中建议观察：

```c
gps_rx_count
gps_line_end_count
gps_last_sentence
gps_last_sentence_ptr
gps_last_rx_status
gps_uart_error_count
gps_uart_error_code
gps_sentence_parse_count
gps_valid_fix_count
gps_latest_fix
```

常用判断：

| 变量现象 | 含义 |
| --- | --- |
| `gps_rx_count == 0` | STM32 没收到 GPS 字节，优先查接线、波特率、供电、共地。 |
| `gps_rx_count` 增加但 `gps_line_end_count == 0` | 收到字节但没收到完整换行，可能波特率不匹配或数据异常。 |
| `gps_last_sentence` 有 `$GPTXT` / `$GPGSA` | 模块有输出，但当前句子不是定位结果。 |
| `gps_sentence_parse_count == 0` | 还没有收到过支持解析的 RMC/GGA 句子。 |
| `gps_sentence_parse_count > 0` 且 `gps_valid_fix_count == 0` | 收到了 RMC/GGA，但内容显示无定位。 |
| `gps_latest_fix.valid == 1` | 当前已有可用定位。 |

最近几条句子的调试变量：

```c
gps_debug_sentence_0_ptr
gps_debug_sentence_1_ptr
gps_debug_sentence_2_ptr
gps_debug_sentence_3_ptr
gps_sentence_history_count
gps_sentence_history_index
```

如果 Watch 不方便显示数组，可优先看：

```c
gps_last_sentence
gps_last_sentence_ptr
```

## 定位有效性说明

RMC 示例：

```text
$GNRMC,083015.00,A,3114.1234,N,12128.5678,E,...
```

状态字段为 `A` 表示有效；`V` 表示无效。

GGA 示例：

```text
$GNGGA,083015.00,3114.1234,N,12128.5678,E,1,08,1.0,42.0,M,...
```

定位质量字段：

| 值 | 含义 |
| --- | --- |
| `0` | 无定位 |
| `1` | GPS/SPS 定位 |
| `2` | DGPS 定位 |
| 其他非 0 | 通常表示某种有效定位，具体含义取决于模块 |

## 主循环中的使用方式

当前主循环逻辑：

```c
if (GPS_ProcessNewSentence(&current_gps_fix))
{
  GPS_UpdateDebugFix(&current_gps_fix);
}

if (gps_latest_fix.valid)
{
  ESP8266_UploadGpsFix(..., &gps_latest_fix, ...);
}
```

上传前会检查：

```c
if (!gps_latest_fix.valid)
{
  gps_upload_skip_invalid_count++;
}
```

因此，只有 RMC/GGA 显示定位有效后，才会进入上传流程。

## 搜星和环境

串口有数据不等于定位有效。若持续看到：

```text
$GPGSA,A,1,...
$BDGSA,A,1,...
```

表示 GPS/北斗都还没有 fix。建议：

1. 初次冷启动放到室外空旷处，天线朝天，等待 1 到 5 分钟。
2. GPS 模块远离 ESP8266、DC-DC、电机、USB 线等干扰源。
3. 确认模块供电稳定，GPS 与 STM32 共地。
4. 确认有源天线插好，板载陶瓷天线正面朝天。
5. 等待 `GPGSA/BDGSA` 第三字段从 `1` 变为 `2` 或 `3`，或等待 `GNRMC` 状态从 `V` 变为 `A`。

## 与上传接口的关系

GPS 定位数据上传接口另见 `API.md`。

本模块只负责：

1. 接收 NMEA。
2. 提取有效定位。
3. 将结果保存到 `gps_latest_fix`。

HTTP 上传由 `esp8266.c` 和 `main.c` 中的上传逻辑负责。
