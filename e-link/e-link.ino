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

#include "wifiConfig.h"
#include "mqtt.h"
#include "epd_gdeh042Z96.h"

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
    EPD_initSPIA();

    // Initialization is complete
    Serial.print("\r\nOk!\r\n");

    // 读取并打印墨水屏硬件ID
    unsigned char i;
    EPD_Reset(); // 复位
    EPD_SendCommand(0x2F); // 读取硬件ID命令
    delay(2);
    digitalWrite(PIN_SPI_DC, HIGH); // 设置 DC 为数据模式，准备读取数据
    i = DEV_SPI_ReadByte();
    Serial.print("EPD: 硬件ID读取结果 = 0x");
    Serial.println(i, HEX);

    // MQTT客户端设置
    mqtt__setup();

    // 图像处理和显示
    EPD_HW_Init(); // 电子纸初始化
    EPD_WhiteScreen_ALL(gImage_BW,gImage_R); // 全屏显示图片（黑白+红色）
    EPD_DeepSleep(); // 进入深度睡眠，降低功耗
    delay(3000);   // 延时3秒
}

/* The main loop -------------------------------------------------------------*/
void loop() 
{
    // The server state observation
    Srvr__loop();

    // WIFI状态检测
    wifi__loop();

    // MQTT循环 检测连接状态并处理消息
    mqtt__loop();
    

}