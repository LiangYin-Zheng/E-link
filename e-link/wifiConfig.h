#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <WiFi.h>
#include "credentials.h" // 包含 WiFi SSID 和密码（隐私，不上传）

// WiFi 账号和密码
// #define WIFI_SSID "your ssid"
// #define WIFI_PASS "your password"



void Wifi__setup() {

    Serial.println();
    Serial.println();
    Serial.print("Connecting to WiFi...");
    Serial.println(WIFI_SSID);

    // wiFi配置
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // 等待连接到路由器
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    // 连接完成
    Serial.println("");
    Serial.println("WiFi connected");
}

void wifi__loop() {
    // 检测WiFi连接状态 如果断开则尝试重新连接
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WARNING] WiFi disconnected. Attempting to reconnect...");
        Wifi__setup();
    }
}

#endif // WIFI_CONFIG_H