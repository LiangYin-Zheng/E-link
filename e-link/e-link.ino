/**
  ******************************************************************************
  * @file    Loader.h
  * @author  Waveshare Team
  * @version V1.0.0
  * @date    23-January-2018
  * @brief   The main file.
  *          This file provides firmware functions:
  *           + Initialization of Serial Port, SPI pins and server
  *           + Main loop
  *
  ******************************************************************************
*/ 

/* Includes ------------------------------------------------------------------*/
#include "srvr.h" // Server functions

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> // 添加 JSON 解析库
#include <HTTPClient.h>
#include <MD5Builder.h>
#include <FS.h>
#include <SPIFFS.h>
// JPEG 解码库（安装库: "JPEGDecoder"）
#include <JPEGDecoder.h>

#define WIFI_SSID "刘"
#define WIFI_PASS "15265531288ll"
#define MQTT_SERVER "broker.emqx.io"
#define MQTT_PORT 1883

WiFiClient espClient;
PubSubClient mqttClient(espClient);

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
    StaticJsonDocument<512> doc; // 定义 JSON 文档对象
    DeserializationError error = deserializeJson(doc, jsonPayload);
    if (error) {
        Serial.print("JSON parse failed: ");
        Serial.println(error.c_str());
        return;
    }

    // 提取 JSON 字段
    const char* type = doc["type"];
    int contentId = doc["contentId"];
    const char* url = doc["url"];
    int size = doc["size"];
    const char* md5 = doc["md5"];
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

    downloadAndVerifyImage(url, md5);
}

void mqttReconnect() {
    const char* deviceCode = "01"; // 示例设备代码，可根据实际情况动态生成
    String topic = "device/" + String(deviceCode) + "/cmd"; // 动态生成主题

    while (!mqttClient.connected()) { // 检查是否已连接
        Serial.print("Attempting MQTT connection...\r\n"); // 添加调试信息
        if (mqttClient.connect("ESP32TestClient")) { // 尝试连接
            Serial.println("MQTT connected\r\n"); // 添加调试信息

            // 连接成功后订阅主题
            if (mqttClient.subscribe(topic.c_str())) { // 动态生成的主题
                Serial.println("Subscribed to topic: " + topic);
            } else {
                Serial.println("Failed to subscribe to topic: " + topic);
            }
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state()); // 打印错误代码
            Serial.println(" try again in 2 seconds");
            delay(2000); // 等待 2 秒后重试
        }
    }
}

/* Entry point ----------------------------------------------------------------*/
void setup() 
{
    // Serial port initialization
    Serial.begin(115200);
    delay(10);

    // SPIFFS initialization
    if (!SPIFFS.begin(true)) {
        Serial.println("[ERROR] An error occurred while mounting SPIFFS.");
        return;
    }

    // Server initialization
    Srvr__setup();

    // SPI initialization
    EPD_initSPI();

    // Initialization is complete
    Serial.print("\r\nOk!\r\n");

    // WiFi初始化
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected");

    // MQTT客户端设置
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    // mqttClient.setCallback(processMqttImageMessage); // 设置新的回调函数
    mqttReconnect();
}

/* The main loop -------------------------------------------------------------*/
void loop() 
{
    // The server state observation
    Srvr__loop();

    // MQTT循环 检测连接状态并处理消息
    if (!mqttClient.connected()) {
        mqttReconnect();
    }
    
    // 处理MQTT消息
    mqttClient.loop();
    
}

// 下载并验证图像数据
void downloadAndVerifyImage(const char* url, const char* expectedMd5) {
    HTTPClient http;
    Serial.println("[INFO] Starting HTTP request..."); // 添加调试信息
    http.begin(url);

    // 发起 HTTP GET 请求
    int httpCode = http.GET();
    Serial.print("[INFO] HTTP GET response code: ");
    Serial.println(httpCode); // 添加调试信息
    if (httpCode != HTTP_CODE_OK) {
        Serial.print("[ERROR] HTTP GET failed: ");
        Serial.println(httpCode);
        return;
    }

    // 获取图像数据流
    WiFiClient* stream = http.getStreamPtr();
    size_t size = http.getSize();
    Serial.print("[INFO] Image size: ");
    Serial.println(size); // 添加调试信息

    uint8_t* buffer = (uint8_t*)malloc(size);
    if (!buffer) {
        Serial.println("[ERROR] Failed to allocate memory for image.");
        return;
    }

    // 读取数据到缓冲区
    Serial.println("[INFO] Reading image data into buffer...");
    stream->readBytes(buffer, size);

    Serial.println("[SUCCESS] Image downloaded and verified successfully.");
    // 解码并只显示黑白到 4.2 寸墨水屏
    displayImageBW(buffer, size);

    // 将图像数据保存到 SPIFFS 中的资源文件夹   
    //saveImageToResources("/image.jpg", buffer, size);
    
    
    
    free(buffer);
    
    // 验证 MD5
    // Serial.println("[INFO] Calculating MD5...");
    // MD5Builder md5;
    // md5.begin();
    // md5.add(buffer, size);
    // md5.calculate();
    // String calculatedMd5 = md5.toString();
    // Serial.print("[INFO] Calculated MD5: ");
    // Serial.println(calculatedMd5); // 添加调试信息

    // if (calculatedMd5 != expectedMd5) {
    //     Serial.println("[ERROR] MD5 mismatch. Image download failed.");
    //     free(buffer);
    //     return;
    // }
}

// 将图像数据保存到 SPIFFS 中的资源文件夹
void saveImageToResources(const char* filename, uint8_t* buffer, size_t size) {
    File file = SPIFFS.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println("[ERROR] Failed to open file for writing.");
        return;
    }
    file.write(buffer, size);
    file.close();
    Serial.println("[SUCCESS] Image saved to resources folder.");

    // 检查文件是否存在
    if (SPIFFS.exists(filename)) {
        Serial.println("[INFO] File successfully saved.");
    } else {
        Serial.println("[ERROR] File not found after saving.");
    }

    // 列出所有文件
    listAllFiles();

    Serial.print("[DEBUG] Free heap: ");
    Serial.println(ESP.getFreeHeap());
}

void listAllFiles() {
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file) {
        Serial.print("File: ");
        Serial.println(file.name());
        file = root.openNextFile();
    }
}

// 仅黑白显示：解码 JPEG -> 缩放 -> 1-bit 黑色缓冲并发送
void displayImageBW(uint8_t* jpgBuf, size_t jpgSize)
{
    const int WIDTH = 400;
    const int HEIGHT = 300;
    const int BYTES_PER_ROW = (WIDTH + 7) / 8; // 50
    const int BUF_SIZE = BYTES_PER_ROW * HEIGHT; // 15000

    uint8_t* bwBuf = (uint8_t*)malloc(BUF_SIZE);
    if (!bwBuf) {
        Serial.println("[ERROR] displayImageBW: malloc failed");
        return;
    }
    memset(bwBuf, 0xFF, BUF_SIZE);

    Serial.println("[DEBUG] Initializing EPD 4in2 V2 (BW)...");
    EPD_Init_4in2_V2();

    Serial.print("[DEBUG] Free heap before decode (bw-only): "); Serial.println(ESP.getFreeHeap());
    if (!JpegDec.decodeArray(jpgBuf, jpgSize)) {
        Serial.println("[ERROR] JPEG decode failed (bw-only)");
        free(bwBuf);
        return;
    }
    Serial.print("[DEBUG] JPEG decoded (bw-only). Image WxH: ");
    Serial.print(JpegDec.width); Serial.print(" x "); Serial.println(JpegDec.height);

    // 为减少缩放时产生的交替噪点，按目标图像行分块处理（stripe），
    // 在每个目标像素块内统计黑/总采样数并按多数投票决定黑或白。
    const int STRIPE_H = 4; // 每次处理的目标行数（可根据内存调整），较小以避免 malloc 失败
    int srcW = JpegDec.width;
    int srcH = JpegDec.height;
    long blackCount = 0;

    for (int stripeY = 0; stripeY < HEIGHT; stripeY += STRIPE_H) {
        int curH = min(STRIPE_H, HEIGHT - stripeY);
        size_t cntSize = (size_t)WIDTH * curH;
        uint8_t* blackCnt = (uint8_t*)malloc(cntSize);
        uint8_t* totalCnt = (uint8_t*)malloc(cntSize);
        if (!blackCnt || !totalCnt) {
            // 尝试退回到每次只处理 1 行以降低内存占用
            if (blackCnt) free(blackCnt);
            if (totalCnt) free(totalCnt);
            int fallbackH = 1;
            size_t fbSize = (size_t)WIDTH * fallbackH;
            blackCnt = (uint8_t*)malloc(fbSize);
            totalCnt = (uint8_t*)malloc(fbSize);
            if (!blackCnt || !totalCnt) {
                Serial.println("[ERROR] displayImageBW: stripe malloc failed (fallback)");
                if (blackCnt) free(blackCnt);
                if (totalCnt) free(totalCnt);
                free(bwBuf);
                return;
            }
            // 调整 curH 与 cntSize 以适配回退大小
            curH = fallbackH;
            cntSize = fbSize;
        }
        memset(blackCnt, 0, cntSize);
        memset(totalCnt, 0, cntSize);

        // 重新解码整张图，并只统计属于当前 stripe 的目标像素采样
        if (!JpegDec.decodeArray(jpgBuf, jpgSize)) {
            Serial.println("[ERROR] JPEG decode failed during stripe");
            free(blackCnt); free(totalCnt); free(bwBuf);
            return;
        }
        while (JpegDec.read()) {
            int mcu_x = JpegDec.MCUx * JpegDec.MCUWidth;
            int mcu_y = JpegDec.MCUy * JpegDec.MCUHeight;
            int mcu_w = JpegDec.MCUWidth;
            int mcu_h = JpegDec.MCUHeight;
            void* pImgVoid = JpegDec.pImage;
            uint8_t* pImage = reinterpret_cast<uint8_t*>(pImgVoid);

            for (int yy = 0; yy < mcu_h; yy++) {
                int py = mcu_y + yy;
                if (py < 0 || py >= srcH) continue;
                int ty = (py * HEIGHT) / srcH;
                if (ty < stripeY || ty >= stripeY + curH) continue;
                for (int xx = 0; xx < mcu_w; xx++) {
                    int px = mcu_x + xx;
                    if (px < 0 || px >= srcW) continue;
                    int idx = (yy * mcu_w + xx) * 3;
                    uint8_t r = pImage[idx + 0];
                    uint8_t g = pImage[idx + 1];
                    uint8_t b = pImage[idx + 2];
                    uint16_t lum = (uint16_t)((r * 299 + g * 587 + b * 114) / 1000);
                    int tx = (px * WIDTH) / srcW;
                    int localY = ty - stripeY;
                    int pos = localY * WIDTH + tx;
                    if (pos < 0 || pos >= (int)cntSize) continue;
                    totalCnt[pos]++;
                    if (lum < 128) blackCnt[pos]++;
                }
            }
        }

        // 根据多数投票决定目标像素，并写入位图
        for (int localY = 0; localY < curH; localY++) {
            int ty = stripeY + localY;
            for (int tx = 0; tx < WIDTH; tx++) {
                int pos = localY * WIDTH + tx;
                if (totalCnt[pos] == 0) continue; // 未采样，保留白
                if ((int)blackCnt[pos] * 2 >= (int)totalCnt[pos]) {
                    int byteIndex = ty * BYTES_PER_ROW + (tx / 8);
                    uint8_t bitMask = 0x80 >> (tx & 7);
                    bwBuf[byteIndex] &= ~bitMask;
                    blackCount++;
                }
            }
        }

        free(blackCnt);
        free(totalCnt);
    }

    Serial.print("[DEBUG] Black pixels marked (bw-only): "); Serial.println(blackCount);

    // 写入并刷新（V2 驱动使用 0x24 写入）
    EPD_SendCommand(0x24);
    delay(2);
    for (int i = 0; i < BUF_SIZE; i++) EPD_SendData(bwBuf[i]);

    EPD_4IN2_V2_Show();

    Serial.print("[DEBUG] Free heap after display (bw-only): "); Serial.println(ESP.getFreeHeap());
    free(bwBuf);
}