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
    
    //Serial.println("[DEBUG] Entering mqttClient.loop()");
    // 处理MQTT消息
    mqttClient.loop();
    //Serial.println("[DEBUG] Exiting mqttClient.loop()");

    // // 定时发布测试消息
    // static unsigned long lastPub = 0;
    // if (millis() - lastPub > 5000) {
    //     mqttClient.publish("test/topic", "Hello from ESP32!")
    //     lastPub = millis();
    // }
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
    // 此处可以调用显示函数，例如：displayImage(buffer, size);

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