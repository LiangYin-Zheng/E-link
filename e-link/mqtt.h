#ifndef MQTT_H
#define MQTT_H

// 提前声明
void mqttCallback(char* topic, byte* payload, unsigned int length);
bool mqttReconnect();

#include <PubSubClient.h>   // MQTT 客户端库
#include <ArduinoJson.h>   // JSON 解析库
#include "image_demo.h" // 图像处理函数声明
#include "epd_gdeh042Z96.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <MD5Builder.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// If PSRAM/heap_caps available, prefer allocating large buffers there
#ifdef MALLOC_CAP_SPIRAM
#include <esp_heap_caps.h>
#endif

// 来自主程序的全局互斥量声明
extern SemaphoreHandle_t displayMutex;

// 外部屏对象（在主程序中定义）
extern EPD_Screen screen1;
extern EPD_Screen screen2;

// Image worker task function (forward declaration)
static void imageWorkerTask(void* param);

// Global reusable buffers to avoid repeated malloc/free (reduces heap fragmentation)
static uint8_t *g_imageBuf = NULL;         // download buffer
static size_t  g_imageBufSize = 0;
static uint8_t *g_displayBuf = NULL;       // BW buffer (ALLSCREEN_GRAGHBYTES)
static uint8_t *g_rBuf = NULL;             // R buffer (ALLSCREEN_GRAGHBYTES)

// allocation helpers (PSRAM preferred if available)
#ifdef MALLOC_CAP_SPIRAM
#define ALLOC_BUF(sz) heap_caps_malloc((sz), MALLOC_CAP_SPIRAM)
#define FREE_BUF(p)   heap_caps_free((p))
#else
#define ALLOC_BUF(sz) malloc((sz))
#define FREE_BUF(p)   free((p))
#endif

// Image task definition (shared between producer and worker)
typedef struct {
  char url[1024];
  char md5[128];
  uint32_t size;
  uint8_t screenId;
} ImageTask;

// Image worker task implementation
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
                http.end();
                continue;
            }
            WiFiClient *stream = http.getStreamPtr();
            size_t contentLen = http.getSize();
            Serial.print("[IMAGE WORKER] Content length: "); Serial.println(contentLen);
            if (contentLen == 0) {
                http.end();
                continue;
            }
            // select target early for possible direct streaming
            EPD_Screen *target = (t.screenId == 2) ? &screen2 : &screen1;

            // Try to allocate display buffers once. If allocation fails, fall back to direct streaming
            if (!g_displayBuf) {
                g_displayBuf = (uint8_t*)ALLOC_BUF(ALLSCREEN_GRAGHBYTES);
                if (g_displayBuf) memset(g_displayBuf, 0xFF, ALLSCREEN_GRAGHBYTES);
            }
            if (!g_rBuf) {
                g_rBuf = (uint8_t*)ALLOC_BUF(ALLSCREEN_GRAGHBYTES);
                if (g_rBuf) memset(g_rBuf, 0x00, ALLSCREEN_GRAGHBYTES);
            }

            size_t totalRead = 0;
            uint8_t tmp[256];

            if (g_displayBuf) {
                // Buffering mode: fill global buffers up to 2*ALLSCREEN_GRAGHBYTES
                while (totalRead < contentLen) {
                    size_t avail = stream->available();
                    if (avail == 0) { delay(10); continue; }
                    size_t toRead = avail > sizeof(tmp) ? sizeof(tmp) : avail;
                    toRead = (toRead > contentLen - totalRead) ? (contentLen - totalRead) : toRead;
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
                }
                http.end();
            } else {
                // Direct streaming mode: write incoming bytes straight to display to avoid allocation
                if (displayMutex != NULL && xSemaphoreTake(displayMutex, pdMS_TO_TICKS(10000)) == pdTRUE) {
                    Serial.println("[IMAGE WORKER] acquired display mutex (direct stream)");
                    EPD_HW_Init_s(target);
                    // Start writing BW RAM
                    Epaper_Write_Command_s(target, 0x24);
                    bool startedR = false;
                    while (totalRead < contentLen) {
                        size_t avail = stream->available();
                        if (avail == 0) { delay(10); continue; }
                        size_t toRead = avail > sizeof(tmp) ? sizeof(tmp) : avail;
                        toRead = (toRead > contentLen - totalRead) ? (contentLen - totalRead) : toRead;
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
                                Epaper_Write_Data_s(target, ~tmp[i]);
                            } else {
                                // beyond display capacity, discard
                            }
                        }
                        totalRead += toRead;
                    }
                    // finalize display
                    EPD_Update_s(target);
                    EPD_DeepSleep_s(target);
                    xSemaphoreGive(displayMutex);
                    Serial.println("[IMAGE WORKER] released display mutex (direct stream)");
                } else {
                    Serial.println("[IMAGE WORKER] failed to acquire display mutex for direct stream, draining");
                    // drain and discard
                    while (totalRead < contentLen) {
                        size_t avail = stream->available();
                        if (avail == 0) { delay(10); continue; }
                        size_t toRead = avail > sizeof(tmp) ? sizeof(tmp) : avail;
                        toRead = (toRead > contentLen - totalRead) ? (contentLen - totalRead) : toRead;
                        stream->readBytes(tmp, toRead);
                        totalRead += toRead;
                    }
                }
                http.end();
            }

            // MD5 and expected-size checks removed per request
            // (data already streamed directly into g_displayBuf/g_rBuf above)

            // If we buffered into g_displayBuf, perform a buffered display now.
            if (g_displayBuf) {
                Serial.println("[IMAGE WORKER] attempting to acquire display mutex");
                if (displayMutex != NULL && xSemaphoreTake(displayMutex, pdMS_TO_TICKS(10000)) == pdTRUE) {
                    Serial.println("[IMAGE WORKER] acquired display mutex");
                    EPD_HW_Init_s(target);
                    EPD_WhiteScreen_ALL_RAM_s(target, g_displayBuf, g_rBuf ? g_rBuf : (const unsigned char*)gImage_R);
                    EPD_DeepSleep_s(target);
                    xSemaphoreGive(displayMutex);
                    Serial.println("[IMAGE WORKER] released display mutex");
                } else {
                    Serial.println("[IMAGE WORKER] failed to acquire display mutex");
                }
            }

            // note: global buffers are reused; do not free here to avoid fragmentation
        }
    }
}


WiFiClient espClient;
PubSubClient mqttClient(espClient);

#define MQTT_SERVER "broker.emqx.io"
#define MQTT_PORT 1883



// MQTT客户端设置
void mqtt__setup() {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    // create image task queue and worker
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        // create queue for image tasks
        // each item: url(512), md5(64), size(uint32_t), screenId(uint8_t)
        // queue can hold up to 4 tasks
        // define in static storage
    }
    mqttReconnect();
}

// MQTT循环 检测连接状态并处理消息
void mqtt__loop() {
    if (!mqttClient.connected()) {
        mqttReconnect();
    }
    mqttClient.loop();
}

// MQTT回调函数
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.println("[DEBUG] mqttCallback triggered."); // 添加调试信息

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