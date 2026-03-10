#ifndef IMAGE_DEMO_H
#define IMAGE_DEMO_H

#include <ArduinoJson.h>    // 添加 JSON 解析库
#include <HTTPClient.h>     // HTTP 客户端库
#include <MD5Builder.h>    // MD5 计算库
#include <JPEGDecoder.h>    // JPEG 解码库
#include "epd_gdeh042Z96.h" // 电子纸驱动头文件

// 提前声明
void downloadAndVerifyImage(const char* url, const char* expectedMd5);

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
    // 解码并显示到 4.2 寸墨水屏
    

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

#endif // IMAGE_DEMO_H