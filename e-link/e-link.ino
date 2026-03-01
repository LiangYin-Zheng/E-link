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

    // MQTT初始化
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
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
