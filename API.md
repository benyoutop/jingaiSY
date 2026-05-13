# 遥测数据上传 API

## 概述

固件通过 ESP8266 发起 HTTP GET 请求，周期性上传超声波测距结果、MPU6050 姿态/运动数据和 MQ-2 烟雾检测数据。如果当前 GPS 定位有效，请同时携带 GPS 数据；如果 GPS 无效，只上传测距、姿态/运动和烟雾数据。

服务端收到合法请求后返回纯文本 `OK`。

## 上传接口

### 请求示例

```http
GET /api/telemetry?device_id=36FF6A063250373439123456&distance_mm=1234&echo_us=7195&tilt_x_mdeg=12345&tilt_y_mdeg=-2300&motion_raw=9200&gyro_x_raw=120&gyro_y_raw=-85&gyro_z_raw=42&smoke_raw=1800&smoke_permille=439&smoke_alarm=0&smoke_do_alarm=0&smoke_ao_alarm=0&valid=1&lat=39908456&lon=116397123&sat=8&alt=42&utc=032519.00 HTTP/1.1
Host: 192.168.1.109:3000
Connection: close
```

### Endpoint

| 项 | 值 |
| --- | --- |
| Method | `GET` |
| Path | `/api/telemetry` |
| Content-Type | `text/plain; charset=utf-8` |

部署到设备或服务器后，`Host` 和端口以实际服务地址为准。开发环境默认地址为 `http://192.168.1.109:3000`。

### Query 参数

| 参数 | 类型 | 必填 | 示例 | 说明 |
| --- | --- | --- | --- | --- |
| `device_id` | string | 是 | `36FF6A063250373439123456` | STM32 96-bit Unique Device ID，格式为 24 位大写十六进制字符串。 |
| `distance_mm` | integer | 是 | `1234` | 超声波测得的距离，单位 mm，必须大于或等于 `0`。 |
| `echo_us` | integer | 是 | `7195` | 超声波 Echo 高电平持续时间，单位 us，必须大于或等于 `0`。 |
| `tilt_x_mdeg` | integer | 是 | `12345` | X 方向倾斜量，单位为毫度。真实角度约为 `tilt_x_mdeg / 1000.0` 度。 |
| `tilt_y_mdeg` | integer | 是 | `-2300` | Y 方向倾斜量，单位为毫度。真实角度约为 `tilt_y_mdeg / 1000.0` 度。 |
| `motion_raw` | integer | 是 | `9200` | MPU6050 原始运动强度，用于判断移动，不是严格物理速度。 |
| `gyro_x_raw` | integer | 是 | `120` | X 轴陀螺仪原始值。 |
| `gyro_y_raw` | integer | 是 | `-85` | Y 轴陀螺仪原始值。 |
| `gyro_z_raw` | integer | 是 | `42` | Z 轴陀螺仪原始值。 |
| `smoke_raw` | integer | 是 | `1800` | MQ-2 AO 的 ADC 原始值，范围通常为 `0` 到 `4095`。 |
| `smoke_permille` | integer | 是 | `439` | MQ-2 相对烟雾浓度比例，范围通常为 `0` 到 `1000`，不等同于 ppm。 |
| `smoke_alarm` | integer | 是 | `0` | MQ-2 最终烟雾报警状态。`1` 表示报警，`0` 表示正常。 |
| `smoke_do_alarm` | integer | 是 | `0` | MQ-2 DO 数字脚是否报警。`1` 表示报警，`0` 表示正常。 |
| `smoke_ao_alarm` | integer | 是 | `0` | MQ-2 AO 模拟值是否超过固件阈值。`1` 表示超过阈值，`0` 表示未超过。 |
| `valid` | integer | 是 | `1` | GPS 定位是否有效。`1` 表示有效，`0` 表示无效。 |
| `lat` | integer | GPS 有效时必填 | `39908456` | 纬度，单位为微度。真实纬度 = `lat / 1000000.0`。范围：`-90000000` 到 `90000000`。 |
| `lon` | integer | GPS 有效时必填 | `116397123` | 经度，单位为微度。真实经度 = `lon / 1000000.0`。范围：`-180000000` 到 `180000000`。 |
| `sat` | integer | GPS 有效时必填 | `8` | 可用卫星数量，必须大于或等于 `0`。 |
| `alt` | integer | GPS 有效时必填 | `42` | 海拔高度，单位 m。 |
| `utc` | string | GPS 有效时必填 | `032519.00` | GPS UTC 时间，来自 NMEA 字段，通常为 `hhmmss.sss`。 |

### GPS 无效时的请求示例

GPS 无效时仍然上传超声波距离、姿态/运动和烟雾数据，只是不携带经纬度等 GPS 字段。

```http
GET /api/telemetry?device_id=36FF6A063250373439123456&distance_mm=1234&echo_us=7195&tilt_x_mdeg=12345&tilt_y_mdeg=-2300&motion_raw=9200&gyro_x_raw=120&gyro_y_raw=-85&gyro_z_raw=42&smoke_raw=1800&smoke_permille=439&smoke_alarm=0&smoke_do_alarm=0&smoke_ao_alarm=0&valid=0 HTTP/1.1
Host: 192.168.1.109:3000
Connection: close
```

### 成功响应

```http
HTTP/1.1 200 OK
Content-Type: text/plain; charset=utf-8

OK
```

### 错误响应

参数缺失或格式错误时返回 `400 Bad Request`，响应体为 JSON。

```http
HTTP/1.1 400 Bad Request
Content-Type: application/json; charset=utf-8

{
  "statusCode": 400,
  "error": "Bad Request",
  "message": "lat is required"
}
```

## 健康检查

用于确认服务是否运行。

```http
GET /health HTTP/1.1
Host: 192.168.1.109:3000
```

成功响应：

```json
{
  "status": "ok"
}
```

## 数据换算

服务端将经纬度整数微度换算为十进制度：

```js
const latitude = lat / 1000000;
const longitude = lon / 1000000;
```

超声波距离由固件按声速约 `343 m/s` 计算：

```text
distance_mm = echo_us * 343 / 2000
```

MPU6050 倾斜量由固件换算为毫度：

```text
tilt_degrees = tilt_x_mdeg / 1000.0
```

`motion_raw` 是加速度变化量和三轴陀螺仪原始值叠加后的运动强度，适合后端判断“是否移动明显”。它不是实际移动速度；如果需要 m/s，需要做校准、滤波和积分，长时间会有漂移。

MQ-2 烟雾浓度当前由 ADC 原始值换算为相对比例：

```text
smoke_permille = smoke_raw * 1000 / 4095
```

`smoke_permille` 用于观察浓度趋势和设置报警阈值，不是严格 ppm。MQ-2 如果要换算真实 ppm，需要先做传感器预热、R0 标定和曲线参数校准。

## 数据存储

服务端将每次合法上传追加写入本地 JSONL 文件：

```text
data/telemetry.jsonl
```

JSONL 是一行一条 JSON 记录，适合设备持续上传数据。新增字段时不需要修改旧记录；服务端会保留原始 query 到 `raw` 字段，方便后续固件增加字段时先完整保存。

记录示例：

```json
{
  "receivedAt": "2026-04-22T01:30:00.000Z",
  "deviceId": "36FF6A063250373439123456",
  "distanceMm": 1234,
  "echoUs": 7195,
  "tilt": {
    "xMdeg": 12345,
    "yMdeg": -2300
  },
  "motionRaw": 9200,
  "gyroRaw": {
    "x": 120,
    "y": -85,
    "z": 42
  },
  "smoke": {
    "raw": 1800,
    "permille": 439,
    "alarm": false,
    "doAlarm": false,
    "aoAlarm": false
  },
  "gps": {
    "valid": true,
    "latitude": 39.908456,
    "longitude": 116.397123,
    "satellites": 8,
    "altitudeM": 42,
    "utc": "032519.00"
  },
  "raw": {
    "device_id": "36FF6A063250373439123456",
    "distance_mm": "1234",
    "echo_us": "7195",
    "tilt_x_mdeg": "12345",
    "tilt_y_mdeg": "-2300",
    "motion_raw": "9200",
    "gyro_x_raw": "120",
    "gyro_y_raw": "-85",
    "gyro_z_raw": "42",
    "smoke_raw": "1800",
    "smoke_permille": "439",
    "smoke_alarm": "0",
    "smoke_do_alarm": "0",
    "smoke_ao_alarm": "0",
    "valid": "1",
    "lat": "39908456",
    "lon": "116397123",
    "sat": "8",
    "alt": "42",
    "utc": "032519.00"
  }
}
```
