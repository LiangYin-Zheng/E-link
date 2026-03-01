/*
 ******************************************************************************
 * @file    epd.h
 * @author  Waveshare Team
 * @version V1.0.0
 * @date    23-January-2018
 * @brief   该文件为墨水屏(e-Paper)驱动头文件，声明了主要的驱动函数和核心变量。
 *
 * 主要函数说明：
 *   void EPD_SendCommand(byte command);                    // 发送命令字节到墨水屏
 *   void EPD_SendData(byte data);                          // 发送数据字节到墨水屏
 *   void EPD_WaitUntilIdle();                              // 等待墨水屏空闲
 *   void EPD_Send_1(byte c, byte v1);                      // 发送带1个参数的命令
 *   void EPD_Send_2(byte c, byte v1, byte v2);             // 发送带2个参数的命令
 *   void EPD_Send_3(byte c, byte v1, byte v2, byte v3);                    // 发送带3个参数的命令
 *   void EPD_Send_4(byte c, byte v1, byte v2, byte v3, byte v4);           // 发送带4个参数的命令
 *   void EPD_Send_5(byte c, byte v1, byte v2, byte v3, byte v4, byte v5);  // 发送带5个参数的命令
 *   void EPD_Reset();                  // 复位墨水屏
 *   void EPD_dispInit();               // 初始化墨水屏，设置型号和相关指针
 *
 * 主要变量说明：
 *   EPD_dispLoad;                // 当前图像加载函数的指针
 *   EPD_dispIndex;               // 当前墨水屏型号的索引
 *   EPD_dispInfo EPD_dispMass[]; // 所有支持墨水屏型号的属性数组
 *
 * 本文件为项目的核心驱动接口
 * ******************************************************************************
 */

#ifndef _EPD_H_
#define _EPD_H_

/* SPI pin definition --------------------------------------------------------*/
#define PIN_SPI_SCK  13
#define PIN_SPI_DIN  14
#define PIN_SPI_CS   15
#define PIN_SPI_BUSY 25//19
#define PIN_SPI_RST  26//21
#define PIN_SPI_DC   27//22

#define PIN_SPI_CS_M    15
#define PIN_SPI_CS_S    2
#define PIN_SPI_PWR     33

/* Pin level definition ------------------------------------------------------*/
#define LOW             0
#define HIGH            1

#define GPIO_PIN_SET    1
#define GPIO_PIN_RESET  0

// =============================
// 函数：EPD_initSPI
// 作用：初始化 SPI 相关引脚，设置为输入/输出，并初始化电平
// =============================
void EPD_initSPI()
{
    pinMode(PIN_SPI_BUSY,  INPUT);      // 设置忙信号引脚为输入
    pinMode(PIN_SPI_RST , OUTPUT);      // 设置复位引脚为输出
    pinMode(PIN_SPI_DC  , OUTPUT);      // 设置数据/命令选择引脚为输出
    
    pinMode(PIN_SPI_SCK, OUTPUT);       // 设置 SPI 时钟引脚为输出
    pinMode(PIN_SPI_DIN, OUTPUT);       // 设置 SPI 数据输入引脚为输出
    pinMode(PIN_SPI_CS , OUTPUT);       // 设置 SPI 片选引脚为输出

    pinMode(PIN_SPI_CS_S , OUTPUT);     // 设置从设备片选为输出
    pinMode(PIN_SPI_PWR , OUTPUT);      // 设置电源控制引脚为输出

    digitalWrite(PIN_SPI_CS, HIGH);     // 片选拉高，默认不选中
    digitalWrite(PIN_SPI_CS_S, HIGH);   // 从片选拉高
    digitalWrite(PIN_SPI_PWR, HIGH);    // 电源拉高，默认上电
    digitalWrite(PIN_SPI_SCK, LOW);     // 时钟拉低，初始状态
}

// =============================
// 函数：GPIO_Mode
// 作用：设置 GPIO 引脚为输入或输出
// =============================
void GPIO_Mode(unsigned char GPIO_Pin, unsigned char Mode)
{
    if(Mode == 0) {
        pinMode(GPIO_Pin , INPUT);   // 0 表示输入
    } else {
        pinMode(GPIO_Pin , OUTPUT);  // 1 表示输出
    }
}

/* Lut mono ------------------------------------------------------------------*/
// 下面是一些预定义的 LUT（查找表）数据，用于控制墨水屏的刷新行为
// 全刷
byte lut_full_mono[] =
{
    0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22, 
    0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88, 
    0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51, 
    0x35, 0x51, 0x51, 0x19, 0x01, 0x00
};
// 局部刷新
byte lut_partial_mono[] =
{
    0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* The procedure of sending a byte to e-Paper by SPI ----
---通过 SPI 协议向电子纸发送一个字节的过程*/

// =============================
// 函数：EpdSpiTransferCallback
// 作用：通过 SPI 手动发送一个字节到 e-Paper
// =============================
void EpdSpiTransferCallback(byte data) 
{
    //SPI.beginTransaction(spi_settings);
    digitalWrite(PIN_SPI_CS, GPIO_PIN_RESET); // 片选拉低，开始传输

    for (int i = 0; i < 8; i++) // 逐位发送 8 位数据
    {
        if ((data & 0x80) == 0)
            digitalWrite(PIN_SPI_DIN, GPIO_PIN_RESET); // 当前位为0，数据线拉低
        else
            digitalWrite(PIN_SPI_DIN, GPIO_PIN_SET);   // 当前位为1，数据线拉高

        data <<= 1;                                 // 左移一位，准备下一个 bit
        digitalWrite(PIN_SPI_SCK, GPIO_PIN_SET);    // 时钟上升沿，发送数据
        digitalWrite(PIN_SPI_SCK, GPIO_PIN_RESET);  // 时钟下降沿，准备下一个 bit
    }
    
    //SPI.transfer(data);
    digitalWrite(PIN_SPI_CS, GPIO_PIN_SET);         // 片选拉高，结束传输
    //SPI.endTransaction();
}


// =============================
// 函数：DEV_SPI_ReadByte
// 作用：通过 SPI 读取一个字节（bit-bang 方式）
// =============================
unsigned char DEV_SPI_ReadByte()
{
    unsigned char j=0xff; // 读取结果，初始为全1
    GPIO_Mode(PIN_SPI_DIN, 0); // 数据线设置为输入
    digitalWrite(PIN_SPI_CS, GPIO_PIN_RESET); // 片选拉低，准备读取
    for (int i = 0; i < 8; i++) // 逐位读取 8 位
    {
        j = j << 1; // 左移一位，为下一个 bit 腾出空间
        if (digitalRead(PIN_SPI_DIN))
            j = j | 0x01; // 读取数据线为高，当前 bit 置1
        else
            j = j & 0xfe; // 读取数据线为低，当前 bit 置0
        digitalWrite(PIN_SPI_SCK, GPIO_PIN_SET);     // 时钟上升沿，采样数据
        digitalWrite(PIN_SPI_SCK, GPIO_PIN_RESET);   // 时钟下降沿
    }
    digitalWrite(PIN_SPI_CS, GPIO_PIN_SET); // 片选拉高，结束读取
    GPIO_Mode(PIN_SPI_DIN, 1);              // 数据线恢复为输出
    return j;                               // 返回读取结果
}

// 下面是一些预定义的 LUT（查找表）数据，用于控制墨水屏的刷新行为
byte lut_vcom0[] = { 15, 0x0E, 0x14, 0x01, 0x0A, 0x06, 0x04, 0x0A, 0x0A, 0x0F, 0x03, 0x03, 0x0C, 0x06, 0x0A, 0x00 };
byte lut_w    [] = { 15, 0x0E, 0x14, 0x01, 0x0A, 0x46, 0x04, 0x8A, 0x4A, 0x0F, 0x83, 0x43, 0x0C, 0x86, 0x0A, 0x04 };
byte lut_b    [] = { 15, 0x0E, 0x14, 0x01, 0x8A, 0x06, 0x04, 0x8A, 0x4A, 0x0F, 0x83, 0x43, 0x0C, 0x06, 0x4A, 0x04 };
byte lut_g1   [] = { 15, 0x8E, 0x94, 0x01, 0x8A, 0x06, 0x04, 0x8A, 0x4A, 0x0F, 0x83, 0x43, 0x0C, 0x06, 0x0A, 0x04 };
byte lut_g2   [] = { 15, 0x8E, 0x94, 0x01, 0x8A, 0x06, 0x04, 0x8A, 0x4A, 0x0F, 0x83, 0x43, 0x0C, 0x06, 0x0A, 0x04 };
byte lut_vcom1[] = { 15, 0x03, 0x1D, 0x01, 0x01, 0x08, 0x23, 0x37, 0x37, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
byte lut_red0 [] = { 15, 0x83, 0x5D, 0x01, 0x81, 0x48, 0x23, 0x77, 0x77, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
byte lut_red1 [] = { 15, 0x03, 0x1D, 0x01, 0x01, 0x08, 0x23, 0x37, 0x37, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/* Sending a byte as a command -----------------------------------------------*/
// =============================
// 函数：EPD_SendCommand
// 作用：发送命令字节到 e-Paper（DC 低）
// =============================
void EPD_SendCommand(byte command) 
{
    digitalWrite(PIN_SPI_DC, LOW);           // DC 低，表示命令
    EpdSpiTransferCallback(command);         // 通过 SPI 发送命令字节
}

// =============================
// 函数：EPD_SendCommand_13in3E6
// 作用：13.3寸 EPD 专用命令发送（bit-bang）
// =============================
void EPD_SendCommand_13in3E6(byte command) 
{
    digitalWrite(PIN_SPI_DC, LOW);           // DC 低，命令
    //SPI.beginTransaction(spi_settings);
    for (int i = 0; i < 8; i++) // 逐位发送 8 位命令
    {
        if ((command & 0x80) == 0)
            digitalWrite(PIN_SPI_DIN, GPIO_PIN_RESET); // 0拉低
        else
            digitalWrite(PIN_SPI_DIN, GPIO_PIN_SET);   // 1拉高

        command <<= 1;                              // 左移一位
        digitalWrite(PIN_SPI_SCK, GPIO_PIN_SET);    // 时钟上升沿
        digitalWrite(PIN_SPI_SCK, GPIO_PIN_RESET);  // 时钟下降沿
    }
    
    //SPI.transfer(command);
    //SPI.endTransaction();
}

/* Sending a byte as a data --------------------------------------------------*/
// =============================
// 函数：EPD_SendData
// 作用：发送数据字节到 e-Paper（DC 高）
// =============================
void EPD_SendData(byte data) 
{
    digitalWrite(PIN_SPI_DC, HIGH);          // DC 高，表示数据
    EpdSpiTransferCallback(data);            // 通过 SPI 发送数据字节
}

// =============================
// 函数：EPD_SendData_13in3E6
// 作用：13.3寸 EPD 专用数据发送（bit-bang）
// =============================
void EPD_SendData_13in3E6(byte data) 
{
    digitalWrite(PIN_SPI_DC, HIGH);          // DC 高，数据
    //SPI.beginTransaction(spi_settings);
    for (int i = 0; i < 8; i++) // 逐位发送 8 位数据
    {
        if ((data & 0x80) == 0)
            digitalWrite(PIN_SPI_DIN, GPIO_PIN_RESET); // 0拉低
        else
            digitalWrite(PIN_SPI_DIN, GPIO_PIN_SET);   // 1拉高

        data <<= 1;                              // 左移一位
        digitalWrite(PIN_SPI_SCK, GPIO_PIN_SET); // 时钟上升沿
        digitalWrite(PIN_SPI_SCK, GPIO_PIN_RESET);// 时钟下降沿
    }
    
    //SPI.transfer(data);
    //SPI.endTransaction();
}

/* Waiting the e-Paper is ready for further instructions ---------------------*/
// =============================
// 函数：EPD_WaitUntilIdle
// 作用：等待 e-Paper 空闲（忙信号为0时继续）
// =============================
void EPD_WaitUntilIdle() 
{
    //0: busy, 1: idle
    while(digitalRead(PIN_SPI_BUSY) == 0) delay(100);    // 忙则等待，每100ms检测一次
}

/* Waiting the e-Paper is ready for further instructions ---------------------*/
// =============================
// 函数：EPD_WaitUntilIdle_high
// 作用：等待 e-Paper 空闲（忙信号为1时继续，部分型号用）
// =============================
void EPD_WaitUntilIdle_high() 
{
    //1: busy, 0: idle
    while(digitalRead(PIN_SPI_BUSY) == 1) delay(100);    // 忙则等待，每100ms检测一次
}

/* Send a one-argument command -----------------------------------------------*/
// =============================
// 函数：EPD_Send_1
// 作用：发送带1个参数的命令
// =============================
void EPD_Send_1(byte c, byte v1)
{
    EPD_SendCommand(c);
    EPD_SendData(v1);
}

/* Send a two-arguments command ----------------------------------------------*/
// =============================
// 函数：EPD_Send_2
// 作用：发送带2个参数的命令
// =============================
void EPD_Send_2(byte c, byte v1, byte v2)
{
    EPD_SendCommand(c);
    EPD_SendData(v1);
    EPD_SendData(v2);
}

/* Send a three-arguments command --------------------------------------------*/
// =============================
// 函数：EPD_Send_3
// 作用：发送带3个参数的命令
// =============================
void EPD_Send_3(byte c, byte v1, byte v2, byte v3)
{
    EPD_SendCommand(c);
    EPD_SendData(v1);
    EPD_SendData(v2);
    EPD_SendData(v3);
}

/* Send a four-arguments command ---------------------------------------------*/
// =============================
// 函数：EPD_Send_4
// 作用：发送带4个参数的命令
// =============================
void EPD_Send_4(byte c, byte v1, byte v2, byte v3, byte v4)
{
    EPD_SendCommand(c);
    EPD_SendData(v1);
    EPD_SendData(v2);
    EPD_SendData(v3);
    EPD_SendData(v4);
}

/* Send a five-arguments command ---------------------------------------------*/
// =============================
// 函数：EPD_Send_5
// 作用：发送带5个参数的命令
// =============================
void EPD_Send_5(byte c, byte v1, byte v2, byte v3, byte v4, byte v5)
{
    EPD_SendCommand(c);
    EPD_SendData(v1);
    EPD_SendData(v2);
    EPD_SendData(v3);
    EPD_SendData(v4);
    EPD_SendData(v5);
}

/* Writting lut-data into the e-Paper ----------------------------------------*/
// =============================
// 函数：EPD_lut
// 作用：写入 LUT 查找表数据到 e-Paper
// =============================
void EPD_lut(byte c, byte l, byte*p)
{
    // lut-data writting initialization
    EPD_SendCommand(c);

    // lut-data writting doing
    for (int i = 0; i < l; i++, p++) EPD_SendData(*p);
}

/* Writting lut-data of the black-white channel ------------------------------*/
// =============================
// 函数：EPD_SetLutBw
// 作用：写入黑白通道的 LUT 查找表
// =============================
void EPD_SetLutBw(byte*c20, byte*c21, byte*c22, byte*c23, byte*c24) 
{
    EPD_lut(0x20, *c20, c20 + 1);//g vcom 
    EPD_lut(0x21, *c21, c21 + 1);//g ww -- 
    EPD_lut(0x22, *c22, c22 + 1);//g bw r
    EPD_lut(0x23, *c23, c23 + 1);//g wb w
    EPD_lut(0x24, *c24, c24 + 1);//g bb b
}

/* Writting lut-data of the red channel --------------------------------------*/
// =============================
// 函数：EPD_SetLutRed
// 作用：写入红色通道的 LUT 查找表
// =============================
void EPD_SetLutRed(byte*c25, byte*c26, byte*c27) 
{
    EPD_lut(0x25, *c25, c25 + 1);
    EPD_lut(0x26, *c26, c26 + 1);
    EPD_lut(0x27, *c27, c27 + 1);
}

/* This function is used to 'wake up" the e-Paper from the deep sleep mode ---*/
// =============================
// 函数：EPD_Reset
// 作用：复位 e-Paper，唤醒出深度睡眠
// =============================
void EPD_Reset() 
{
    digitalWrite(PIN_SPI_RST, HIGH); 
    delay(200);    
    digitalWrite(PIN_SPI_RST, LOW);    
    delay(2);
    digitalWrite(PIN_SPI_RST, HIGH); 
    delay(200);    
}

/* e-Paper initialization functions ------------------------------------------*/ 
#include "epd1in54.h"
#include "epd2in13.h"
 #include "epd2in9.h"
// #include "epd2in7.h"
//#include "epd2in66.h"
// #include "epd3in7.h"
// #include "epd3in52.h"
#include "epd4in01f.h"
//#include "epd4in26.h"
// #include "epd5in65f.h"
// #include "epd5in83.h"
// #include "epd7in3.h"
// #include "epd7in5.h"
// #include "epd7in5_HD.h"
#include "epd13in3.h"

// 作用：图像数据是否需要按位取反（部分型号或特殊显示模式下需要）
// true 表示写入墨水屏前需对每个字节按位取反
bool EPD_invert;           // 若为 true，图像数据写入前需按位取反

// 作用：当前所选墨水屏型号在型号数组中的索引
// 用于切换不同尺寸/类型的墨水屏驱动参数
int  EPD_dispIndex;        // 当前墨水屏型号的索引

// 作用：当前像素的 X、Y 坐标（仅 2.13 寸等部分型号需要）
// 用于逐行/逐列写入像素数据时定位
int  EPD_dispX, EPD_dispY; // 当前像素坐标（部分型号用）

// 作用：当前图像数据加载函数的函数指针
// 根据型号自动指向对应的写入函数，实现多型号兼容
void(*EPD_dispLoad)();     // 当前图像加载函数指针

/* Image data loading function for a-type e-Paper ------- a 型墨水屏的图像数据加载函数---*/ 

void EPD_loadA()
{
    // Come back to the image data end
    Buff__bufInd -= 8;

    // Get the index of the image data begin
    int pos = Buff__bufInd - Buff__getWord(Buff__bufInd);

    // Enumerate all of image data bytes
    while (pos < Buff__bufInd)
    {
        // Get current byte
        int value = Buff__getByte(pos);
//		Serial.printf("address:%d, data:%x ",pos, value);
//		if(pos % 8 == 0)
//			Serial.printf("\r\n");
        // Invert byte's bits in case of '2.7' e-Paper
        if (EPD_invert) value = ~value;

        // Write the byte into e-Paper's memory
        EPD_SendData((byte)value);

        // Increment the current byte index on 2 characters
        pos += 2;
    }
}

void EPD_loadAFilp()
{
    // Come back to the image data end
    Buff__bufInd -= 8;

    // Get the index of the image data begin
    int pos = Buff__bufInd - Buff__getWord(Buff__bufInd);
    
    // Enumerate all of image data bytes
    while (pos < Buff__bufInd)
    {
        // Get current byte
        int value = Buff__getByte(pos);
		
        // Invert byte's bits in case of '2.7' e-Paper
        if (EPD_invert) value = ~value;

        // Write the byte into e-Paper's memory
        EPD_SendData(~(byte)value);

        // Increment the current byte index on 2 characters
        pos += 2;
    }
}

/* Image data loading function for b-type e-Paper ----------------------------*/
void EPD_loadB()
{
    // Come back to the image data end
    Buff__bufInd -= 8;

    // Get the index of the image data begin
    int pos = Buff__bufInd - Buff__getWord(Buff__bufInd);

    // Enumerate all of image data bytes
    while (pos < Buff__bufInd)
    {
        // Get current word from obtained image data
        int valueA = (int)Buff__getByte(pos) + ((int)Buff__getByte(pos + 2) << 8);

        // Clean current word of processed image data 
        int valueB = 0;

        // Enumerate next 8 pixels
        for (int p = 0; p < 8; p++)
        {
            // Current obtained pixel data
            int pixInd = valueA & 3;

            // Remove the obtained pixel data from 'valueA' word
            valueA = valueA >> 2;

            // Processing of 8 2-bit pixels to 8 2-bit pixels: 
            // black(value 0) to bits 00, white(value 1) to bits 11, gray(otherwise) to bits 10
            valueB = (valueB << 2) + (pixInd == 1 ? 3 : (pixInd == 0 ? 0 : 2));
        }

        // Write the word into e-Paper's memory
        EPD_SendData((byte)(valueB >> 8));
        EPD_SendData((byte)valueB);

        // Increment the current byte index on 2 characters
        pos += 4;
    }
}

/* Image data loading function for 2.13 e-Paper ------------------------------*/
void EPD_loadC()
{
    // Come back to the image data end
    Buff__bufInd -= 8;

    // Get the index of the image data begin
    int pos = Buff__bufInd - Buff__getWord(Buff__bufInd);

    EPD_Send_2(0x44, 0, 15);        //SET_RAM_X_ADDRESS_START_END_POSITION LO(x >> 3), LO((w - 1) >> 3)
    EPD_Send_4(0x45, 0, 0, 249, 0); //SET_RAM_Y_ADDRESS_START_END_POSITION LO(y), HI(y), LO(h - 1), HI(h - 1)

    // Enumerate all of image data bytes
    while (pos < Buff__bufInd)
    {
        // Before write a line of image data
        // 2.13 e-Paper requires to set the address counter
        // Every line has 15*8-6 pixels + 6 empty bits, totally 15*8 bits
        if (EPD_dispX == 0)
        {
            EPD_Send_1(0x4E, 0           );//SET_RAM_X_ADDRESS_COUNTER: LO(x >> 3)
            EPD_Send_2(0x4F, EPD_dispY, 0);//SET_RAM_Y_ADDRESS_COUNTER: LO(y), HI(y)  
            EPD_SendCommand(0x24);//WRITE_RAM
        }

        // Write the byte into e-Paper's memory
        EPD_SendData((byte)Buff__getByte(pos));  

        // Increment the current byte index on 2 characters     
        pos += 2;

        // EPD_dispX and EPD_dispY increments
        if (++EPD_dispX > 15)
        {
            EPD_dispX = 0;

            // If the client's browser sends more bits, than it needs, then exit the function
            if (++EPD_dispY > 250) return;
        }
    }
}

/* Image data loading function for 7.5 e-Paper -------------------------------*/
void EPD_loadD()
{
    // Come back to the image data end
    Buff__bufInd -= 8;

    // Get the index of the image data begin
    int pos = Buff__bufInd - Buff__getWord(Buff__bufInd);

    // Enumerate all of image data bytes
    while (pos < Buff__bufInd)
    {
        // Get current byte from obtained image data
        int valueA = Buff__getByte(pos);

        // Processing of 4 1-bit pixels to 4 4-bit pixels:
        // black(value 0) to bits 0000, white(value 1) to bits 0011
        EPD_SendData((byte)((valueA & 0x80) ? 0x30 : 0x00) + ((valueA & 0x40) ? 0x03 : 0x00));
        EPD_SendData((byte)((valueA & 0x20) ? 0x30 : 0x00) + ((valueA & 0x10) ? 0x03 : 0x00));
        EPD_SendData((byte)((valueA & 0x08) ? 0x30 : 0x00) + ((valueA & 0x04) ? 0x03 : 0x00));
        EPD_SendData((byte)((valueA & 0x02) ? 0x30 : 0x00) + ((valueA & 0x01) ? 0x03 : 0x00));  

        // Increment the current byte index on 2 characters 
        pos += 2;
    }
}

/* Image data loading function for 7.5b e-Paper ------------------------------*/
void EPD_loadE()
{
    // Come back to the image data end
    Buff__bufInd -= 8;

    // Get the index of the image data begin
    int pos = Buff__bufInd - Buff__getWord(Buff__bufInd);

    // Enumerate all of image data bytes
    while (pos < Buff__bufInd)
    {
        // Get current byte from obtained image data
        int value = Buff__getByte(pos);  

        // Processing of 4 1-bit pixels to 4 4-bit pixels:
        // red(value 1) to bits 0011, white(value 3) to bits 0100
        int A = (value     ) & 3;if (A == 3) A = 4;if (A == 1) A = 3;
        int B = (value >> 2) & 3;if (B == 3) B = 4;if (B == 1) B = 3;
        int C = (value >> 4) & 3;if (C == 3) C = 4;if (C == 1) C = 3;
        int D = (value >> 6) & 3;if (D == 3) D = 4;if (D == 1) D = 3;

        // Write the word into e-Paper's memory
        EPD_SendData((A << 4) + B);
        EPD_SendData((C << 4) + D);

        // Increment the current byte index on 2 characters
        pos += 2;
    }
}

/* Image data loading function for 5.83b e-Paper -----------------------------*/
void EPD_loadF()
{
    // Come back to the image data end
    Buff__bufInd -= 8;

    // Get the index of the image data begin
    int pos = Buff__bufInd - Buff__getWord(Buff__bufInd);

    // Enumerate all of image data bytes
    while (pos < Buff__bufInd)
    {
        // Get current byte from obtained image data
        int value = Buff__getByte(pos);  
		int value1 = Buff__getByte(pos+2);  
        // Processing of 4 1-bit pixels to 4 4-bit pixels:
        // white(value 1) to bits 0011, red(value 2) to bits 0100
        int A = (value     ) & 3;if (A == 2) A = 4;
        int B = (value >> 8) & 3;if (B == 2) B = 4;
        int C = (value1 >> 4) & 3;if (C == 2) C = 4;
        int D = (value1 >> 6) & 3;if (D == 2) D = 4;

        // Write the word into e-Paper's memory
        EPD_SendData((A << 4) + B);
        EPD_SendData((C << 4) + D);

        // Increment the current byte index on 2 characters
        pos += 4;
    }
}

/* Image data loading function for 5.65f e-Paper -----------------------------*/
void EPD_loadG()
{
    // Come back to the image data end
    Buff__bufInd -= 8;

    // Get the index of the image data begin
    int pos = Buff__bufInd - Buff__getWord(Buff__bufInd);

    // Enumerate all of image data bytes
    while (pos < Buff__bufInd)
    {
        // Get current byte from obtained image data
        int value = Buff__getByte(pos);  
		
        // Switch the positions of the two 4-bits pixels
        // Black:0b000;White:0b001;Green:0b010;Blue:0b011;Red:0b100;Yellow:0b101;Orange:0b110;
        int A = (value     ) & 0x07;
        int B = (value >> 4) & 0x07;
		
        // Write the data into e-Paper's memory
        EPD_SendData((byte)(A << 4) + B);
		
        // Increment the current byte index on 2 characters
        pos += 2;
    }
}

void EPD_load_13in3E6()
{
    // Come back to the image data end
    Buff__bufInd -= 8;

    // Get the index of the image data begin
    int pos = Buff__bufInd - Buff__getWord(Buff__bufInd);

    // Enumerate all of image data bytes
    while (pos < Buff__bufInd)
    {
        // Get current byte from obtained image data
        int value = Buff__getByte(pos);  
		
        // Switch the positions of the two 4-bits pixels
        // Black:0b000;White:0b001;Green:0b010;Blue:0b011;Red:0b100;Yellow:0b101;Orange:0b110;
        int A = (value     ) & 0x07;
        int B = (value >> 4) & 0x07;
		
        // Write the data into e-Paper's memory
        EPD_SendData_13in3E6((byte)(A << 4) + B);
		
        // Increment the current byte index on 2 characters
        pos += 2;
    }
}

/* Show image and turn to deep sleep mode (a-type, 4.2 and 2.7 e-Paper) ------*/
/*
 * 显示图像并进入深度睡眠模式（适用于A型、4.2寸和2.7寸墨水屏）
 * 主要流程：
 * 1. 刷新显示内容。
 * 2. 等待刷新完成。
 * 3. 发送休眠命令，使墨水屏进入低功耗状态。
 * 适用范围：A型、4.2寸、2.7寸等常见黑白墨水屏。
 */
void EPD_showA() 
{
    // Refresh 刷新显示内容
    // 设置显示更新控制寄存器，0xC4通常表示全刷+关电等操作
    EPD_Send_1(0x22, 0xC4);// DISPLAY_UPDATE_CONTROL_2

    // 发送主激活命令，触发显示刷新
    EPD_SendCommand( 0x20);// MASTER_ACTIVATION

    // 等待显示刷新完成，墨水屏忙信号变为0时继续
    EPD_SendCommand( 0xFF);// TERMINATE_FRAME_READ_WRITE

    // 等待墨水屏空闲，确保刷新完成
    EPD_WaitUntilIdle();

    // Sleep  进入睡眠模式
    // 发送电源关闭命令，准备进入睡眠模式
    EPD_SendCommand(0x10);// DEEP_SLEEP_MODE

    // 等待墨水屏空闲，确保进入睡眠模式。
    EPD_WaitUntilIdle();
}

/* Show image and turn to deep sleep mode (b-type, e-Paper) ------------------*/
/*
 * 显示图像并进入深度睡眠模式（适用于B型墨水屏）
 * 主要流程：
 * 1. 刷新显示内容。
 * 2. 等待刷新完成。
 * 3. 设置休眠相关寄存器，准备进入低功耗状态。
 * 4. 发送电源关闭命令，进入睡眠模式。
 * 适用范围：B型墨水屏，如部分双色墨水屏。
 */
void EPD_showB() 
{
    // Refresh
    EPD_SendCommand(0x12);// DISPLAY_REFRESH
    delay(100);
    EPD_WaitUntilIdle();

    // Sleep
    EPD_Send_1(0x50, 0x17);// VCOM_AND_DATA_INTERVAL_SETTING
    EPD_Send_1(0x82, 0x00);// VCM_DC_SETTING_REGISTER, to solve Vcom drop
    EPD_Send_4(0x01, 0x02, 0x00, 0x00, 0x00);// POWER_SETTING
    EPD_WaitUntilIdle();
    EPD_SendCommand(0x02);// POWER_OFF
}

/* Show image and turn to deep sleep mode (7.5 and 7.5b e-Paper) -------------*/
void EPD_showC() 
{
    // Refresh
    EPD_SendCommand(0x12);// DISPLAY_REFRESH
    delay(100);
    EPD_WaitUntilIdle();

    // Sleep
    EPD_SendCommand(0x02);// POWER_OFF
    EPD_WaitUntilIdle();
    EPD_Send_1(0x07, 0xA5);// DEEP_SLEEP
}

void EPD_showD()
{
    // VCOM AND DATA INTERVAL SETTING
    // WBmode:VBDF 17, D7 VBDW 97, VBDB 57
    // WBRmode:VBDF F7, VBDW 77, VBDB 37, VBDR B7
    EPD_Send_1(0x50, 0x97);

    EPD_SendCommand(0x20);
    for(int count=0; count<44; count++) EPD_SendData(lut_vcomDC_2in13d[count]);
    
    EPD_SendCommand(0x21);
    for(int count=0; count<42; count++) EPD_SendData(lut_ww_2in13d[count]);

    EPD_SendCommand(0x22);
    for(int count=0; count<42; count++) EPD_SendData(lut_bw_2in13d[count]);
    
    EPD_SendCommand(0x23);
    for(int count=0; count<42; count++) EPD_SendData(lut_wb_2in13d[count]);
    
    EPD_SendCommand(0x24);
    for(int count=0; count<42; count++) EPD_SendData(lut_bb_2in13d[count]);

    delay(10);
    EPD_SendCommand(0x12);//DISPLAY REFRESH
    delay(100);     //!!!The delay here is necessary, 200uS at least!!!
    EPD_WaitUntilIdle(); 

    EPD_Send_1(0x50, 0xf7);
    EPD_SendCommand( 0x02);//POWER_OFF
    EPD_Send_1(0x07, 0xA5);//DEEP_SLEEP
}

#include "epd4in2.h"

/* The set of pointers on 'init', 'load' and 'show' functions, title and code */
/*
    * 'init'：指向对应型号墨水屏的初始化函数
    * 'chBk'：指向黑色通道图像数据加载函数
    * 'next'：切换通道代码，-1表示无切换，0x13表示切换到红色通道
    * 'chRd'：指向红色通道图像数据加载函数（如果适用）
    * 'show'：指向显示并进入睡眠模式的函数
    * 'title'：墨水屏型号的标题字符串
*/
struct EPD_dispInfo // 描述墨水屏驱动参数的结构体
{
    int(*init)(); // Initialization
    void(*chBk)();// Black channel loading
    int next;     // Change channel code
    void(*chRd)();// Red channel loading
    void(*show)();// Show and sleep
    char*title;   // Title of an e-Paper
};


/* Array of sets describing the usage of e-Papers ----------------------------*/
/*
    * 该数组包含多个元素，每个元素描述一种墨水屏型号的驱动参数和函数指针。
    * 通过修改 EPD_dispIndex 的值，可以切换不同型号的墨水屏驱动。
    * 每个元素包括：
    * - init：初始化函数指针
    * - chBk：黑色通道图像数据加载函数指针
    * - next：切换通道代码，-1表示无切换，0x13表示切换到红色通道
    * - chRd：红色通道图像数据加载函数指针（如果适用）
    * - show：显示并进入睡眠模式的函数指针
    * - title：墨水屏型号的标题字符串
*/
EPD_dispInfo EPD_dispMass[] =
{
    { EPD_Init_1in54,		EPD_loadA,		-1  ,	0,				EPD_1IN54_Show,		"1.54 inch"		},// a 0
    { EPD_Init_1in54b,		EPD_loadB,		0x13,	EPD_loadA,		EPD_showB,			"1.54 inch b"	},// b 1
    { EPD_Init_1in54c,		EPD_loadA,		0x13,	EPD_loadA,		EPD_showB,			"1.54 inch c"	},// c 2
    { EPD_Init_2in13,		EPD_loadC,		-1  ,	0,				EPD_showA,			"2.13 inch"		},// d 3
    { EPD_Init_2in13b,		EPD_loadA,		0x13,	EPD_loadA,		EPD_showB,			"2.13 inch b"	},// e 4
    { EPD_Init_2in13b,		EPD_loadA,		0x13,	EPD_loadA,		EPD_showB,			"2.13 inch c"	},// f 5
    { EPD_Init_2in13d,		EPD_loadA,		-1  ,	0,				EPD_showD,			"2.13 inch d"	},// g 6
// { EPD_Init_2in7,		EPD_loadA, 		1  ,	0,				EPD_showB,			"2.7 inch"		},// h 7
// { EPD_Init_2in7b,		EPD_loadA,		0x13,	EPD_loadA,		EPD_showB,			"2.7 inch b"	},// i 8
// { EPD_Init_2in9,		EPD_loadA,		-1  ,	0,				EPD_showA,			"2.9 inch"		},// j 9
// { EPD_Init_2in9b,		EPD_loadA,		0x13,	EPD_loadA,		EPD_showB,			"2.9 inch b"	},// k 10
// { EPD_Init_2in9b,		EPD_loadA,		0x13,	EPD_loadA,		EPD_showB,			"2.9 inch c"	},// l 11
// { EPD_Init_2in9d,		EPD_loadA,		-1  ,	0,				EPD_2IN9D_Show,		"2.9 inch d"	},// M 12
    { EPD_Init_4in2b_V2,	EPD_loadA,		-1  ,	0,				EPD_4IN2B_V2_Show,	"4.2 inch"		},// N 13 -> 4.2 黑白完美版
    { EPD_Init_4in2b_V2,	EPD_loadA,		0x13,	EPD_4IN2B_V2_load,		EPD_4IN2B_V2_Show,	"4.2 inch b"	},// O 14 -> 4.2b 黑白红三色完美版
    { EPD_Init_4in2b,		EPD_loadA,		0x13,	EPD_loadA,		EPD_showB,			"4.2 inch c"	},// P 15
// { EPD_5in83__init,		EPD_loadD,		-1  ,	0,				EPD_showC,			"5.83 inch"		},// Q 16
// { EPD_5in83b__init,		EPD_loadE,		-1  ,	0,				EPD_showC,			"5.83 inch b"	},// R 17
// { EPD_5in83b__init,		EPD_loadE,		-1  ,	0,				EPD_showC,			"5.83 inch c"	},// S 18
// { EPD_7in5__init,		EPD_loadD,		-1  ,	0,				EPD_showC,			"7.5 inch"		},// T 19    
// { EPD_7in5__init,		EPD_loadE,		-1  ,	0,				EPD_showC,			"7.5 inch b"	},// u 20
// { EPD_7in5__init,		EPD_loadE,		-1  ,	0,				EPD_showC,			"7.5 inch c"	},// v 21
// { EPD_7in5_V2_init,		EPD_loadAFilp,	-1  ,	0,				EPD_7IN5_V2_Show,	"7.5 inch V2"	},// w 22
// { EPD_7in5B_V2_Init,	EPD_loadA,		0x13,	EPD_loadAFilp,	EPD_7IN5_V2_Show,	"7.5 inch B V2"	},// x 23
// { EPD_7IN5B_HD_init,	EPD_loadA,		0x26,	EPD_loadAFilp,	EPD_7IN5B_HD_Show,	"7.5 inch B HD"	},// y 24
// { EPD_5IN65F_init,		EPD_loadG,		-1  ,	0,				EPD_5IN65F_Show,	"5.65 inch F "	},// z 25
// { EPD_7IN5_HD_init,		EPD_loadA,		-1	,	0,				EPD_7IN5_HD_Show,	"7.5 inch HD"	},// A 26
// { EPD_3IN7_1Gray_Init,	EPD_loadA,		-1	,	0,				EPD_3IN7_1Gray_Show,"3.7 inch"		},// 27
// { EPD_2IN66_Init,		EPD_loadA,		-1	,	0,				EPD_2IN66_Show,		"2.66 inch"		},// 28
// { EPD_5in83b_V2_init,	EPD_loadA,		0x13,	EPD_loadAFilp,	EPD_showC,			"5.83 inch B V2"},// 29
// { EPD_Init_2in9b_V3,	EPD_loadA,		0x13,	EPD_loadA,		EPD_showC,			"2.9 inch B V3"	},// 30
// { EPD_1IN54B_V2_Init,	EPD_loadA,		0x26,	EPD_loadAFilp,	EPD_1IN54B_V2_Show,	"1.54 inch B V2"},// 31
// { EPD_2IN13B_V3_Init,	EPD_loadA,		0x13,	EPD_loadA,		EPD_2IN13B_V3_Show,	"2.13 inch B V3"},// 32
// { EPD_Init_2in9_V2, 	EPD_loadA,		-1,		0,				EPD_2IN9_V2_Show,	"2.9 inch V2"	},// 33
    { EPD_Init_4in2b_V2,	EPD_loadA,		-1  ,	EPD_4IN2B_V2_load,EPD_4IN2B_V2_Show,"4.2 inch b V2"	},// 34
// { EPD_2IN66B_Init,		EPD_loadA,		0x26,	EPD_loadAFilp,	EPD_2IN66_Show,		"2.66 inch b"	},// 35
// { EPD_Init_5in83_V2,	EPD_loadAFilp,	-1,		0,				EPD_showC,			"5.83 inch V2"	},// 36
// { EPD_4IN01F_init,		EPD_loadG,		-1,		0,				EPD_4IN01F_Show,	"4.01 inch f"	},// 37
// { EPD_Init_2in7b_V2,	EPD_loadA,		0x26,	EPD_loadAFilp,	EPD_Show_2in7b_V2,	"2.7 inch B V2"	},// 38
// { EPD_Init_2in13_V3,	EPD_loadC,		-1, 	0, 				EPD_2IN13_V3_Show, 	"2.13 inch V3"	},// 39
// { EPD_2IN13B_V4_Init,	EPD_loadA,		0x26,	EPD_loadA,		EPD_2IN13B_V4_Show, "2.13 inch B V4"},// 40
// { EPD_3IN52_Init,	    EPD_loadA,		-1,	    0,		        EPD_3IN52_Show,     "3.52 inch"     },// 41
// { EPD_2IN7_V2_Init,		EPD_loadA, 		-1  ,	0,				EPD_2IN7_V2_Show,	"2.7 inch V2"	},// 42
// { EPD_Init_2in13_V4,	EPD_loadC, 		-1  ,	0,				EPD_2IN13_V4_Show,	"2.13 inch V4"	},// 43
    { EPD_Init_4in2_V2,	    EPD_loadA, 		-1  ,	0,				EPD_4IN2_V2_Show,	"4.2 inch V2"	},// 44
// { EPD_13in3k_init,	    EPD_loadA, 		-1  ,	0,				EPD_13in3k_Show,	"13.3 inch K"	},// 45
// { EPD_4in26_init,	    EPD_loadA, 		-1  ,	0,				EPD_4in26_Show,	    "4.26 inch"	    },// 46
// { EPD_Init_2in9b_V4,    EPD_loadA, 		0x26,	EPD_loadAFilp,	EPD_2in9b_V4_Show,	"2.9 inch b V4" },// 47
// { EPD_13in3b_init,      EPD_loadA, 		0x26,	EPD_loadAFilp,	EPD_13in3b_Show ,	"13.3 inch b"   },// 48
// { EPD_7in3E_init,       EPD_loadG, 		-1,	    0,	            EPD_7in3E_Show ,	"7.3 inch E"    },// 49
// { EPD_13in3E_init,      EPD_load_13in3E6, -1,   EPD_load_13in3E6, EPD_13in3E_Show , "13.3 inch E"   },// 50
};

/* Initialization of an e-Paper ----------------------------------------------*/
// =============================
// 函数：EPD_dispInit
// 作用：初始化 e-Paper，设置当前型号的初始化、加载、显示函数指针
// =============================
void EPD_dispInit()
{
    // Call initialization function
    EPD_dispMass[EPD_dispIndex].init(); // 调用当前索引对应的墨水屏初始化函数

    // Set loading function for black channel
    EPD_dispLoad = EPD_dispMass[EPD_dispIndex].chBk; // 设置当前图像加载函数指针为黑色通道加载函数

    // Set initial coordinates
    // 对于部分型号，初始化当前像素坐标为(0, 0)，准备逐行/逐列写入图像数据
    EPD_dispX = 0; // 初始化当前像素坐标X为0
    EPD_dispY = 0; // 初始化当前像素坐标Y为0

    // The inversion of image data bits isn't needed by default
    EPD_invert = false;
}

#endif // _EPD_H_