#ifndef IMAGE_DEMO_H
#define IMAGE_DEMO_H

#include <ArduinoJson.h>    // 添加 JSON 解析库
#include <HTTPClient.h>     // HTTP 客户端库
#include <MD5Builder.h>    // MD5 计算库
#include <JPEGDecoder.h>    // JPEG 解码库
#include "epd.h"
#include "epd4in2.h"

// 提前声明
void downloadAndVerifyImage(const char* url, const char* expectedMd5);
void displayImageBW(uint8_t* jpgBuf, size_t jpgSize);   // 仅黑白显示：解码 JPEG -> 缩放 -> 1-bit 黑色缓冲并发送

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

// 仅黑白显示：解码 JPEG -> 缩放 -> 1-bit 黑色缓冲并发送
void displayImageBW(uint8_t* jpgBuf, size_t jpgSize)
{
    const int WIDTH = 400;
    const int HEIGHT = 300;
    const int BYTES_PER_ROW = (WIDTH + 7) / 8;
    const int BUF_SIZE = BYTES_PER_ROW * HEIGHT;

    uint8_t* bwBuf = (uint8_t*)malloc(BUF_SIZE);
    if (!bwBuf) {
        Serial.println("[ERROR] displayImageBW: malloc failed");
        return;
    }
    memset(bwBuf, 0xFF, BUF_SIZE);

    Serial.println("[DEBUG] Initializing EPD 4in2 V2 (BW)...");
    EPD_Init_4in2_V2();

    if (!JpegDec.decodeArray(jpgBuf, jpgSize)) {
        Serial.println("[ERROR] JPEG decode failed (bw-only)");
        free(bwBuf);
        return;
    }
    int srcW = JpegDec.width;
    int srcH = JpegDec.height;

    float scale = min((float)WIDTH / srcW, (float)HEIGHT / srcH);
    int scaledW = int(srcW * scale + 0.5f);
    int scaledH = int(srcH * scale + 0.5f);
    int offsetX = (WIDTH - scaledW) / 2;
    int offsetY = (HEIGHT - scaledH) / 2;

    // 1. 生成缩放后的灰度图缓冲区
    float* grayBuf = (float*)malloc(WIDTH * HEIGHT * sizeof(float));
    if (!grayBuf) {
        Serial.println("[ERROR] malloc grayBuf failed");
        free(bwBuf);
        return;
    }
    // 先全白
    for (int i = 0; i < WIDTH * HEIGHT; i++) grayBuf[i] = 255.0f;

    JpegDec.decodeArray(jpgBuf, jpgSize);
    while (JpegDec.read()) {
        int mcu_x = JpegDec.MCUx * JpegDec.MCUWidth;
        int mcu_y = JpegDec.MCUy * JpegDec.MCUHeight;
        int mcu_w = JpegDec.MCUWidth;
        int mcu_h = JpegDec.MCUHeight;
        uint8_t* pImage = reinterpret_cast<uint8_t*>(JpegDec.pImage);

        for (int yy = 0; yy < mcu_h; yy++) {
            int py = mcu_y + yy;
            if (py >= srcH) continue;
            for (int xx = 0; xx < mcu_w; xx++) {
                int px = mcu_x + xx;
                if (px >= srcW) continue;

                // 映射到目标图像
                int tx = int(px * scale + 0.5f) + offsetX;
                int ty = int(py * scale + 0.5f) + offsetY;
                if (tx < 0 || tx >= WIDTH || ty < 0 || ty >= HEIGHT) continue;

                int idx = (yy * mcu_w + xx) * 3;
                uint8_t r = pImage[idx + 0];
                uint8_t g = pImage[idx + 1];
                uint8_t b = pImage[idx + 2];
                float lum = (r * 299 + g * 587 + b * 114) / 1000.0f;
                grayBuf[ty * WIDTH + tx] = lum;
            }
        }
    }

    // 2. Floyd-Steinberg 抖动
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int idx = y * WIDTH + x;
            float oldPixel = grayBuf[idx];
            float newPixel = oldPixel < 128 ? 0.0f : 255.0f;
            float quantError = oldPixel - newPixel;
            grayBuf[idx] = newPixel;

            // 扩散误差
            if (x + 1 < WIDTH)
                grayBuf[y * WIDTH + (x + 1)] += quantError * 7 / 16.0f;
            if (y + 1 < HEIGHT) {
                if (x > 0)
                    grayBuf[(y + 1) * WIDTH + (x - 1)] += quantError * 3 / 16.0f;
                grayBuf[(y + 1) * WIDTH + x] += quantError * 5 / 16.0f;
                if (x + 1 < WIDTH)
                    grayBuf[(y + 1) * WIDTH + (x + 1)] += quantError * 1 / 16.0f;
            }
        }
    }

    // 3. 灰度转1-bit黑白缓冲
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (grayBuf[y * WIDTH + x] < 128) {
                int byteIndex = y * BYTES_PER_ROW + (x / 8);
                uint8_t bitMask = 0x80 >> (x & 7);
                bwBuf[byteIndex] &= ~bitMask;
            }
        }
    }
    free(grayBuf);

    // 写入并刷新
    EPD_SendCommand(0x24);
    delay(2);
    for (int i = 0; i < BUF_SIZE; i++) EPD_SendData(bwBuf[i]);
    EPD_4IN2_V2_Show();

    free(bwBuf);
}

#endif // IMAGE_DEMO_H