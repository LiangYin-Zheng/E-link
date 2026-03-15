#ifndef MQTT_H
#define MQTT_H

// 提前声明
void mqttCallback(char* topic, byte* payload, unsigned int length);
bool mqttReconnect();

// 前置声明：在本文件较早处会调用这些上报函数，实际实现位于 mqtt_status.h
void mqtt_report_status(const char* deviceCode, const char* status, const char* messageId, const char* error, unsigned long long timestamp);
void mqtt_send_heartbeat(const char* deviceCode, const char* status, int currentContentId, const char* currentContentType, int battery, int signal, unsigned long long timestamp);

#define MQTT_MAX_PACKET_SIZE 2048
#include <PubSubClient.h>   // MQTT 客户端库
#include <ArduinoJson.h>    // JSON 解析库
#include "image.h"     // 图像处理函数声明
#include "epd_gdeh042Z96.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <MD5Builder.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// 设备代码（与订阅命令的主题一致）
#define DEVICE_CODE "01"

// If PSRAM/heap_caps available, prefer allocating large buffers there
#ifdef MALLOC_CAP_SPIRAM
#include <esp_heap_caps.h>
#endif

// 来自主程序的全局互斥量声明
extern SemaphoreHandle_t displayMutex;

// 外部屏对象（在主程序中定义）
extern EPD_Screen screen1;
extern EPD_Screen screen2;

// 内部函数声明
static void imageWorkerTask(void* param);

// 全局缓冲区指针（如果分配失败则为NULL，改为直接流式处理）
static uint8_t *g_imageBuf = NULL;         // download buffer
static size_t  g_imageBufSize = 0;
static uint8_t *g_displayBuf = NULL;       // BW buffer (ALLSCREEN_GRAGHBYTES)
static uint8_t *g_rBuf = NULL;             // R buffer (ALLSCREEN_GRAGHBYTES)

typedef struct {
  char url[1024];
  char md5[128];
    char messageId[128];
  uint32_t size;
  uint8_t screenId;
} ImageTask;

// 图像下载与显示工作线程
static void imageWorkerTask(void* param) {
    QueueHandle_t q = (QueueHandle_t)param;
    ImageTask t;
    for(;;) {
        if (xQueueReceive(q, &t, portMAX_DELAY) == pdTRUE) {
            Serial.println("[IMAGE WORKER] Got task, downloading...");
            HTTPClient http;
            http.begin(t.url);
            int httpCode = http.GET();
                    if (httpCode != HTTP_CODE_OK) {
                        Serial.print("[IMAGE WORKER] HTTP GET failed: "); Serial.println(httpCode);
                        // report failure
                        mqtt_report_status(DEVICE_CODE, "FAILURE", t.messageId[0] ? t.messageId : "", "HTTP_GET_FAILED", 0);
                        http.end();
                        continue;
                    }
            WiFiClient *stream = http.getStreamPtr();
            long contentLenLL = http.getSize();
            Serial.print("[IMAGE WORKER] HTTP Content-Length: "); Serial.println(contentLenLL);

            // Determine expected length: prefer task.size, then Content-Length, else assume full 2*frame
            size_t expectedLen = 0;
            if (t.size > 0) expectedLen = t.size;
            else if (contentLenLL > 0) expectedLen = (size_t)contentLenLL;
            else expectedLen = 2 * ALLSCREEN_GRAGHBYTES; // fallback when unknown

            // Cap expected length to avoid unbounded reads
            if (expectedLen > 2 * ALLSCREEN_GRAGHBYTES) expectedLen = 2 * ALLSCREEN_GRAGHBYTES;

            Serial.print("[IMAGE WORKER] expected bytes to read: "); Serial.println(expectedLen);
            // select target early for possible direct streaming
            EPD_Screen *target = (t.screenId == 2) ? &screen2 : &screen1;

            size_t totalRead = 0;
            uint8_t tmp[256];
            // Try lazy allocation of display buffers (prefer PSRAM when available)
            if (!g_displayBuf) {
#ifdef MALLOC_CAP_SPIRAM
                g_displayBuf = (uint8_t*)heap_caps_malloc(ALLSCREEN_GRAGHBYTES, MALLOC_CAP_SPIRAM);
                g_rBuf = (uint8_t*)heap_caps_malloc(ALLSCREEN_GRAGHBYTES, MALLOC_CAP_SPIRAM);
#else
                g_displayBuf = (uint8_t*)malloc(ALLSCREEN_GRAGHBYTES);
                g_rBuf = (uint8_t*)malloc(ALLSCREEN_GRAGHBYTES);
#endif
                if (g_displayBuf) memset(g_displayBuf, 0xFF, ALLSCREEN_GRAGHBYTES);
                if (g_rBuf) memset(g_rBuf, 0x00, ALLSCREEN_GRAGHBYTES);
            }

            if (g_displayBuf) {
                // Buffering mode: global buffers are ready

                // Buffering mode: fill global buffers up to expectedLen (safe for unknown/ chunked transfers)
                uint32_t idleStart = millis();
                const uint32_t READ_IDLE_TIMEOUT = 15000; // ms
                while (totalRead < expectedLen) {
                    size_t avail = stream->available();
                    if (avail == 0) {
                        if (millis() - idleStart > READ_IDLE_TIMEOUT) {
                            Serial.println("[IMAGE WORKER] read idle timeout (buffered)");
                            break;
                        }
                        delay(10);
                        continue;
                    }
                    idleStart = millis();
                    size_t toRead = avail > sizeof(tmp) ? sizeof(tmp) : avail;
                    toRead = (toRead > expectedLen - totalRead) ? (expectedLen - totalRead) : toRead;
                    stream->readBytes(tmp, toRead);
                    // copy into BW and R sections as appropriate
                    for (size_t i = 0; i < toRead; i++) {
                        size_t pos = totalRead + i;
                        if (pos < (size_t)ALLSCREEN_GRAGHBYTES) {
                            g_displayBuf[pos] = tmp[i];
                        } else if (pos < (size_t)(2 * ALLSCREEN_GRAGHBYTES) && g_rBuf) {
                            g_rBuf[pos - ALLSCREEN_GRAGHBYTES] = tmp[i];
                        } else {
                            // beyond our interest, discard
                        }
                    }
                    totalRead += toRead;
                    taskYIELD();
                }

                // If we read less than expected, fill remaining with defaults
                if (totalRead < (size_t)ALLSCREEN_GRAGHBYTES) {
                    size_t remain = ALLSCREEN_GRAGHBYTES - totalRead;
                    if (g_displayBuf) memset(g_displayBuf + totalRead, 0xFF, remain);
                }
                if (totalRead < (size_t)(2 * ALLSCREEN_GRAGHBYTES) && g_rBuf) {
                    size_t rstart = (totalRead > ALLSCREEN_GRAGHBYTES) ? (totalRead - ALLSCREEN_GRAGHBYTES) : 0;
                    if (rstart < ALLSCREEN_GRAGHBYTES) {
                        size_t remainR = ALLSCREEN_GRAGHBYTES - rstart;
                        memset(g_rBuf + rstart, 0x00, remainR);
                    }
                }

                http.end();
            } else if (expectedLen >= ALLSCREEN_GRAGHBYTES) {
                // Direct streaming only when we expect at least one full BW frame worth of data.
                // This avoids partially writing the display when server returns only a small payload (e.g. a compressed image).
                if (displayMutex != NULL && xSemaphoreTake(displayMutex, pdMS_TO_TICKS(10000)) == pdTRUE) {
                    Serial.println("[IMAGE WORKER] acquired display mutex (direct stream)");
                    EPD_HW_Init_s(target);
                    // Start writing BW RAM
                    Epaper_Write_Command_s(target, 0x24);
                    bool startedR = false;
                    uint32_t idleStart = millis();
                    const uint32_t READ_IDLE_TIMEOUT = 15000; // ms
                    while (totalRead < expectedLen) {
                        size_t avail = stream->available();
                        if (avail == 0) {
                            if (millis() - idleStart > READ_IDLE_TIMEOUT) {
                                Serial.println("[IMAGE WORKER] read idle timeout (direct)");
                                break;
                            }
                            delay(10);
                            continue;
                        }
                        idleStart = millis();
                        size_t toRead = avail > sizeof(tmp) ? sizeof(tmp) : avail;
                        toRead = (toRead > expectedLen - totalRead) ? (expectedLen - totalRead) : toRead;
                        stream->readBytes(tmp, toRead);
                        for (size_t i = 0; i < toRead; i++) {
                            size_t pos = totalRead + i;
                            if (pos < (size_t)ALLSCREEN_GRAGHBYTES) {
                                Epaper_Write_Data_s(target, tmp[i]);
                            } else if (pos < (size_t)(2 * ALLSCREEN_GRAGHBYTES)) {
                                if (!startedR) {
                                    Epaper_Write_Command_s(target, 0x26);
                                    startedR = true;
                                }
                                Epaper_Write_Data_s(target, tmp[i]);
                            } else {
                                // beyond display capacity, discard
                            }
                            if ((i & 0xFF) == 0) taskYIELD();
                        }
                        totalRead += toRead;
                        taskYIELD();
                    }

                    // If stream ended early, pad remaining bytes on-device
                    if (totalRead < (size_t)ALLSCREEN_GRAGHBYTES) {
                        size_t pad = ALLSCREEN_GRAGHBYTES - totalRead;
                        for (size_t i = 0; i < pad; i++) Epaper_Write_Data_s(target, 0xFF);
                        totalRead = ALLSCREEN_GRAGHBYTES;
                    }
                    if (totalRead < (size_t)(2 * ALLSCREEN_GRAGHBYTES)) {
                        if (!startedR) { Epaper_Write_Command_s(target, 0x26); startedR = true; }
                        size_t rpad = (2 * ALLSCREEN_GRAGHBYTES) - totalRead;
                        for (size_t i = 0; i < rpad; i++) Epaper_Write_Data_s(target, 0x00);
                        totalRead = 2 * ALLSCREEN_GRAGHBYTES;
                    }
                    // finalize display
                    EPD_Update_s(target);
                    EPD_DeepSleep_s(target);
                    xSemaphoreGive(displayMutex);
                    Serial.println("[IMAGE WORKER] released display mutex (direct stream)");
                                        // report success to backend
                                        mqtt_report_status(DEVICE_CODE, "SUCCESS", t.messageId[0] ? t.messageId : "", "", 0);
                } else {
                    Serial.println("[IMAGE WORKER] failed to acquire display mutex for direct stream, draining");
                    // drain and discard using expectedLen as total length
                    while (totalRead < expectedLen) {
                        size_t avail = stream->available();
                        if (avail == 0) { delay(10); continue; }
                        size_t toRead = avail > sizeof(tmp) ? sizeof(tmp) : avail;
                        toRead = (toRead > expectedLen - totalRead) ? (expectedLen - totalRead) : toRead;
                        stream->readBytes(tmp, toRead);
                        totalRead += toRead;
                    }
                }
                http.end();
            } else {
                // Small payloads (e.g. compressed JPEG) — download into temporary buffer instead of direct streaming.
                Serial.println("[IMAGE WORKER] small payload, downloading to temp buffer (no partial display)");
                if (!g_imageBuf || g_imageBufSize < expectedLen) {
                    if (g_imageBuf) free(g_imageBuf);
                    g_imageBuf = (uint8_t*)malloc(expectedLen);
                    if (g_imageBuf) g_imageBufSize = expectedLen; else g_imageBufSize = 0;
                }
                if (g_imageBuf) {
                    uint32_t idleStart = millis();
                    const uint32_t READ_IDLE_TIMEOUT = 15000; // ms
                    while (totalRead < expectedLen) {
                        size_t avail = stream->available();
                        if (avail == 0) {
                            if (millis() - idleStart > READ_IDLE_TIMEOUT) {
                                Serial.println("[IMAGE WORKER] read idle timeout (temp)");
                                break;
                            }
                            delay(10);
                            continue;
                        }
                        idleStart = millis();
                        size_t toRead = avail > sizeof(tmp) ? sizeof(tmp) : avail;
                        toRead = (toRead > expectedLen - totalRead) ? (expectedLen - totalRead) : toRead;
                        stream->readBytes(tmp, toRead);
                        memcpy(g_imageBuf + totalRead, tmp, toRead);
                        totalRead += toRead;
                        taskYIELD();
                    }
                } else {
                    // failed to allocate temp buffer — drain stream and skip
                    Serial.println("[IMAGE WORKER] failed to allocate temp buffer, draining stream");
                    while (totalRead < expectedLen) {
                        size_t avail = stream->available();
                        if (avail == 0) { delay(10); continue; }
                        size_t toRead = avail > sizeof(tmp) ? sizeof(tmp) : avail;
                        toRead = (toRead > expectedLen - totalRead) ? (expectedLen - totalRead) : toRead;
                        stream->readBytes(tmp, toRead);
                        totalRead += toRead;
                    }
                }
                http.end();
                // Post-download: if the payload equals one or two full frames, copy into display buffers and show.
                if (g_imageBuf && g_displayBuf == NULL) {
#ifdef MALLOC_CAP_SPIRAM
                    g_displayBuf = (uint8_t*)heap_caps_malloc(ALLSCREEN_GRAGHBYTES, MALLOC_CAP_SPIRAM);
                    g_rBuf = (uint8_t*)heap_caps_malloc(ALLSCREEN_GRAGHBYTES, MALLOC_CAP_SPIRAM);
#else
                    g_displayBuf = (uint8_t*)malloc(ALLSCREEN_GRAGHBYTES);
                    g_rBuf = (uint8_t*)malloc(ALLSCREEN_GRAGHBYTES);
#endif
                    if (g_displayBuf) memset(g_displayBuf, 0xFF, ALLSCREEN_GRAGHBYTES);
                    if (g_rBuf) memset(g_rBuf, 0x00, ALLSCREEN_GRAGHBYTES);
                }
                if (g_imageBuf && g_displayBuf) {
                    if (totalRead >= (size_t)(2 * ALLSCREEN_GRAGHBYTES)) {
                        memcpy(g_displayBuf, g_imageBuf, ALLSCREEN_GRAGHBYTES);
                        memcpy(g_rBuf, g_imageBuf + ALLSCREEN_GRAGHBYTES, ALLSCREEN_GRAGHBYTES);
                    } else if (totalRead >= (size_t)ALLSCREEN_GRAGHBYTES) {
                        // only BW provided
                        memcpy(g_displayBuf, g_imageBuf, ALLSCREEN_GRAGHBYTES);
                        if (g_rBuf) memset(g_rBuf, 0x00, ALLSCREEN_GRAGHBYTES);
                    } else {
                        // unknown/smaller payload (likely JPEG) — cannot display as raw frames
                        if (totalRead >= 2 && g_imageBuf[0] == 0xFF && g_imageBuf[1] == 0xD8) {
                            Serial.println("[IMAGE WORKER] JPEG payload received — server-side conversion required or implement JPEG decoder");
                        } else {
                            Serial.println("[IMAGE WORKER] payload too small and not raw frame — skipping display");
                        }
                    }
                    // If we have full frames in buffers, show them
                    if (g_displayBuf) {
                        Serial.println("[IMAGE WORKER] attempting to acquire display mutex (buffered post-download)");
                        if (displayMutex != NULL && xSemaphoreTake(displayMutex, pdMS_TO_TICKS(10000)) == pdTRUE) {
                            Serial.println("[IMAGE WORKER] acquired display mutex");
                            EPD_HW_Init_s(target);
                            EPD_WhiteScreen_ALL_RAM_s(target, g_displayBuf, g_rBuf ? g_rBuf : (const unsigned char*)gImage_R);
                            EPD_DeepSleep_s(target);
                            xSemaphoreGive(displayMutex);
                            Serial.println("[IMAGE WORKER] released display mutex");
                                                        // report success to backend
                                                        mqtt_report_status(DEVICE_CODE, "SUCCESS", t.messageId[0] ? t.messageId : "", "", 0);
                        } else {
                            Serial.println("[IMAGE WORKER] failed to acquire display mutex");
                        }
                    }
                }
            }

            if (g_displayBuf) {
                Serial.println("[IMAGE WORKER] attempting to acquire display mutex (buffered)");
                if (displayMutex != NULL && xSemaphoreTake(displayMutex, pdMS_TO_TICKS(10000)) == pdTRUE) {
                    Serial.println("[IMAGE WORKER] acquired display mutex");

                    // Optional MD5 check if provided
                    if (t.md5[0] != '\0' && g_rBuf) {
                        MD5Builder md5;
                        md5.begin();
                        md5.add(g_displayBuf, ALLSCREEN_GRAGHBYTES);
                        md5.add(g_rBuf, ALLSCREEN_GRAGHBYTES);
                        md5.calculate();
                        String got = md5.toString();
                        if (got != String(t.md5)) {
                            Serial.print("[IMAGE WORKER] MD5 mismatch: "); Serial.println(got);
                            // still allow display but warn
                        } else {
                            Serial.println("[IMAGE WORKER] MD5 matched");
                        }
                    }

                                        EPD_HW_Init_s(target);
                                        EPD_WhiteScreen_ALL_RAM_s(target, g_displayBuf, g_rBuf ? g_rBuf : (const unsigned char*)gImage_R);
                                        EPD_DeepSleep_s(target);
                                        xSemaphoreGive(displayMutex);
                                        Serial.println("[IMAGE WORKER] released display mutex");
                                        // report success to backend
                                        // report success to backend
                                        mqtt_report_status(DEVICE_CODE, "SUCCESS", t.messageId[0] ? t.messageId : "", "", 0);
                } else {
                    Serial.println("[IMAGE WORKER] failed to acquire display mutex");
                }
            }

        }
    }
}

WiFiClient espClient;
PubSubClient mqttClient(espClient);

#include "mqtt_status.h"

#define MQTT_SERVER "broker.emqx.io"
#define MQTT_PORT 1883

// MQTT客户端设置
void mqtt__setup() {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    Serial.print("[MQTT] server: "); Serial.print(MQTT_SERVER); Serial.print(":"); Serial.println(MQTT_PORT);
    // create image task queue and worker
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
    }
    mqttReconnect();
}

// MQTT循环 检测连接状态并处理消息
void mqtt__loop() {
    if (!mqttClient.connected()) {
        Serial.println("[MQTT] not connected, attempting reconnect...");
        mqttReconnect();
    }
    mqttClient.loop();
}

// MQTT回调函数
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.println("[DEBUG] mqttCallback triggered."); // 添加调试信息
    Serial.print("[DEBUG] payload length: "); Serial.println(length);

    Serial.print("[MQTT] Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    // 将 payload 转换为字符串
    String jsonPayload = String((char*)payload).substring(0, length);

    // 解析 JSON 数据
    StaticJsonDocument<2048> doc; // 定义 JSON 文档对象，扩大以容纳长 URL
    DeserializationError error = deserializeJson(doc, jsonPayload);
    if (error) {
        Serial.print("JSON parse failed: ");
        Serial.println(error.c_str());
        return;
    }

    // 提取 JSON 字段
    // 字段校验
    if (!doc.containsKey("type") || !doc.containsKey("url")) {
        Serial.println("MQTT payload missing required fields");
        return;
    }

    const char* type = doc["type"];
    int contentId = doc.containsKey("contentId") ? doc["contentId"].as<int>() : -1;
    const char* url = doc["url"];
    uint32_t size = doc.containsKey("size") ? doc["size"].as<uint32_t>() : 0;
    const char* md5 = doc.containsKey("md5") ? doc["md5"].as<const char*>() : NULL;
    long long timestamp = doc["timestamp"];
    const char* messageId = doc["messageId"];

    // 打印提取的字段
    Serial.println("Parsed JSON:");
    Serial.print("Type: "); Serial.println(type);
    Serial.print("Content ID: "); Serial.println(contentId);
    Serial.print("URL: "); Serial.println(url);
    Serial.print("Size: "); Serial.println(size);
    Serial.print("MD5: "); Serial.println(md5);
    Serial.print("Timestamp: "); Serial.println(timestamp);
    Serial.print("Message ID: "); Serial.println(messageId);

    // 解析要显示到哪块屏（默认 screen1）并将任务入队，由后台任务执行下载与显示
    // 则把 contentId 当作屏幕ID（后端只发送 contentId 时使用）
    uint8_t screenId = 1;
    if (doc.containsKey("screenId")) screenId = doc["screenId"].as<uint8_t>();
    else if (doc.containsKey("id")) screenId = doc["id"].as<uint8_t>();
    else if (contentId >= 0) screenId = (uint8_t)contentId;

    // Prepare simple task object
    ImageTask task;
    memset(&task,0,sizeof(task));
    strncpy(task.url, url, sizeof(task.url)-1);
    if (md5) strncpy(task.md5, md5, sizeof(task.md5)-1);
    if (messageId) strncpy(task.messageId, messageId, sizeof(task.messageId)-1);
    task.size = size;
    task.screenId = screenId;

    // Lazy-create queue and worker if not already
    static QueueHandle_t imageQueue = NULL;
    if (imageQueue == NULL) {
        imageQueue = xQueueCreate(4, sizeof(ImageTask));
        if (imageQueue == NULL) Serial.println("[ERROR] Failed to create imageQueue");
        else {
            // create worker task
            xTaskCreate(
                imageWorkerTask,
                "ImageWorker",
                8192,
                imageQueue,
                1,
                NULL
            );
        }
    }

    // try to enqueue without blocking
    if (imageQueue != NULL) {
        if (xQueueSend(imageQueue, &task, 0) != pdTRUE) {
            Serial.println("[MQTT] imageQueue full, dropping task");
        } else {
            Serial.println("[MQTT] image task queued");
        }
    } else {
        Serial.println("[MQTT] no image queue available");
    }
}

// Try to connect once and return success
bool mqttReconnect() {
    const char* deviceCode = "01"; // 示例设备代码，可根据实际情况动态生成
    String topic = "device/" + String(deviceCode) + "/cmd"; // 动态生成主题

    if (mqttClient.connected()) return true;

    // generate unique client id from MAC
    String clientId = "ESP32-" + WiFi.macAddress();
    Serial.print("Attempting MQTT connection with clientId: "); Serial.println(clientId);
    if (mqttClient.connect(clientId.c_str())) {
        Serial.println("MQTT connected");
        if (mqttClient.subscribe(topic.c_str())) {
            Serial.print("Subscribed to topic: "); Serial.println(topic);
        } else {
            Serial.print("Failed to subscribe to topic: "); Serial.println(topic);
        }
        return true;
    } else {
        Serial.print("MQTT connect failed, rc="); Serial.println(mqttClient.state());
        return false;
    }
}

#endif // MQTT_H