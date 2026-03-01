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

    // Server initialization
    // Srvr__setup();

    // SPI initialization
    EPD_initSPI();

    // Initialization is complete
    Serial.print("\r\nOk!\r\n");

    // WiFi初始化
    Wifi__setup();

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