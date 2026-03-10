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
//#include "srvr.h" // Server functions

#include "wifiConfig.h"
#include "mqtt.h"
#include "epd_gdeh042Z96.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// 全局显示互斥量，防止同时对两块屏幕并发刷新
SemaphoreHandle_t displayMutex = NULL;

bool takeDisplay(uint32_t msTimeout = 5000) {
  if (displayMutex == NULL) return false;
  return xSemaphoreTake(displayMutex, pdMS_TO_TICKS(msTimeout)) == pdTRUE;
}

void giveDisplay() {
  if (displayMutex) xSemaphoreGive(displayMutex);
}

// Define two EPD screen instances (调整为你的实际引脚)
EPD_Screen screen1 = { .RES = 5,  .DC = 17, .CS = 16, .BUSY = 4,  .PWR = 255 }; // PWR=255表示不使用电源控制
EPD_Screen screen2 = { .RES = 25, .DC = 26, .CS = 27, .BUSY = 14, .PWR = 255 }; // 第二块屏的引脚配置

/* Entry point ----------------------------------------------------------------*/
void setup() 
{
    // Serial port initialization
    Serial.begin(115200);
    delay(10);

    // WiFi初始化
    Wifi__setup();

    // Server initialization
    // Srvr__setup();

    // SPI initialization
    EPD_initSPI(); // 初始化共享 SPI 引脚

    // 初始化每块屏的控制引脚
    EPD_initPins(&screen1);
    EPD_initPins(&screen2);

    // Initialization is complete
    Serial.print("\r\nOk!\r\n");

    // // 读取并打印墨水屏硬件ID
    // unsigned char i;
    // EPD_Reset(); // 复位
    // EPD_SendCommand(0x2F); // 读取硬件ID命令
    // delay(2);
    // digitalWrite(PIN_SPI_DC, HIGH); // 设置 DC 为数据模式，准备读取数据
    // i = DEV_SPI_ReadByte();
    // Serial.print("EPD: 硬件ID读取结果 = 0x");
    // Serial.println(i, HEX);

    // MQTT客户端设置
    mqtt__setup();

    // 创建显示互斥量，防止并发刷新
    displayMutex = xSemaphoreCreateMutex();
    if (displayMutex == NULL) {
      Serial.println("[ERROR] failed to create display mutex");
    } else {
      Serial.println("[INFO] display mutex created");
    }

    // 初始化并显示屏1（示例）
    if (takeDisplay(10000)) {
      Serial.println("[INFO] init/display screen1");
      EPD_HW_Init_s(&screen1);
      EPD_WhiteScreen_ALL_s(&screen1, gImage_BW, gImage_R);
      EPD_DeepSleep_s(&screen1);
      giveDisplay();
    } else {
      Serial.println("[WARN] could not lock display for screen1 init");
    }

    // 初始化屏2并清屏（示例）
    if (takeDisplay(10000)) {
      Serial.println("[INFO] init/display screen2");
      EPD_HW_Init_s(&screen2);
      EPD_WhiteScreen_ALL_s(&screen2, gImage_BW, gImage_R);
      // EPD_WhiteScreen_ALL_Clean_s(&screen2);
      EPD_DeepSleep_s(&screen2);
      giveDisplay();
    } else {
      Serial.println("[WARN] could not lock display for screen2 init");
    }
    delay(3000);   // 延时3秒
}

/* The main loop -------------------------------------------------------------*/
void loop() 
{
    // The server state observation
    // Srvr__loop();

    // WIFI状态检测
    wifi__loop();

    // MQTT循环 检测连接状态并处理消息
    mqtt__loop();
    

}