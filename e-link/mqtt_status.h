#ifndef MQTT_STATUS_H
#define MQTT_STATUS_H

#include <ArduinoJson.h>
#include <time.h>

// 发布设备状态（成功/失败）
// device/{deviceCode}/status
// {
//   "status": "SUCCESS",
//   "messageId": "uuid-string",
//   "error": "错误信息（失败时才有）",
//   "timestamp": 1704614400000
// }
void mqtt_report_status(const char* deviceCode, const char* status, const char* messageId, const char* error, unsigned long long timestamp = 0) {
    if (!mqttClient.connected()) {
        Serial.println("[MQTT] cannot publish status, not connected");
        return;
    }

    StaticJsonDocument<256> doc;
    if (status) doc["status"] = status; else doc["status"] = "";
    if (messageId) doc["messageId"] = messageId; else doc["messageId"] = "";
    if (error && strlen(error) > 0) doc["error"] = error;

    if (timestamp == 0) {
        time_t t = time(nullptr);
        unsigned long long ts = (unsigned long long)t * 1000ULL;
        doc["timestamp"] = ts;
    } else {
        doc["timestamp"] = timestamp;
    }

    char buf[512];
    size_t len = serializeJson(doc, buf, sizeof(buf));
    String topic = String("device/") + String(deviceCode) + String("/status");
    if (mqttClient.publish(topic.c_str(), buf, len)) {
        Serial.println("显示成功");
    } else {
        Serial.println("显示失败");
    }
}

// 发布心跳消息
// device/{deviceCode}/heartbeat
// {
//   "deviceCode": "ESP32_001",
//   "status": "ONLINE",
//   "currentContentId": 123,
//   "currentContentType": "IMAGE",
//   "battery": 85,
//   "signal": -65,
//   "timestamp": 1704614400000
// }
void mqtt_send_heartbeat(const char* deviceCode, const char* status, int currentContentId, const char* currentContentType, int battery, int signal, unsigned long long timestamp = 0) {
    if (!mqttClient.connected()) {
        Serial.println("[MQTT] cannot publish heartbeat, not connected");
        return;
    }

    StaticJsonDocument<384> doc;
    if (deviceCode) doc["deviceCode"] = deviceCode; else doc["deviceCode"] = "";
    if (status) doc["status"] = status; else doc["status"] = "";
    doc["currentContentId"] = currentContentId;
    if (currentContentType) doc["currentContentType"] = currentContentType; else doc["currentContentType"] = "";
    doc["battery"] = battery;
    doc["signal"] = signal;

    if (timestamp == 0) {
        time_t t = time(nullptr);
        unsigned long long ts = (unsigned long long)t * 1000ULL;
        doc["timestamp"] = ts;
    } else {
        doc["timestamp"] = timestamp;
    }

    char buf[768];
    size_t len = serializeJson(doc, buf, sizeof(buf));
    String topic = String("device/") + String(deviceCode) + String("/heartbeat");
    if (mqttClient.publish(topic.c_str(), buf, len)) {
        Serial.println("状态信息已发送");
    } else {
        Serial.println("状态信息发送失败");
    }
}

#endif // MQTT_STATUS_H
