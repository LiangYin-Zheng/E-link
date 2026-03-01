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
#include "epd.h"
#include "epd4in2.h"

/* Entry point ----------------------------------------------------------------*/
void setup() 
{
    // Serial port initialization
    Serial.begin(115200);
    delay(10);

    // WiFi初始化
    Wifi__setup();

    // Server initialization
    Srvr__setup();

    // SPI initialization
    EPD_initSPI();

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