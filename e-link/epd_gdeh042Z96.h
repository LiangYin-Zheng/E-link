#ifndef __EPD_GDEH042Z96_H_
#define __EPD_GDEH042Z96_H_

#include "Ap_29demo.h" // 包含图片数据头文件

/* SPI pin definition --------------------------------------------------------*/
#define SPI_SCK     13      // SPI时钟引脚（SCK），输出
#define SPI_DIN     14      // SPI数据输入引脚（DIN/MOSI），输出
#define SPI_CS      15      // SPI片选引脚（CS），输出
#define SPI_BUSY    25      // 电子纸忙信号（BUSY），输入，用于检测屏幕是否空闲
#define SPI_RES     26      // 复位引脚（RST），输出
#define SPI_DC      27      // 数据/命令选择引脚（DC），输出

#define SPI_CS_M    5  // 主设备片选引脚
#define SPI_CS_S    2  // 从设备片选引脚
#define SPI_PWR     33 // 电源控制引脚


/* SPI Pin level definition ------------------------------------------------------*/
#define EPD_W21_MOSI_0  digitalWrite(SPI_DIN,LOW)   // MOSI输出低电平
#define EPD_W21_MOSI_1  digitalWrite(SPI_DIN,HIGH)  // MOSI输出高电平

#define EPD_W21_CLK_0   digitalWrite(SPI_SCK,LOW)   // SCK输出低电平
#define EPD_W21_CLK_1   digitalWrite(SPI_SCK,HIGH)  // SCK输出高电平

#define EPD_W21_CS_0    digitalWrite(SPI_CS,LOW)    // CS输出低电平（选中）
#define EPD_W21_CS_1    digitalWrite(SPI_CS,HIGH)   // CS输出高电平（释放）

#define EPD_W21_DC_0    digitalWrite(SPI_DC,LOW)    // DC输出低电平（命令）
#define EPD_W21_DC_1    digitalWrite(SPI_DC,HIGH)   // DC输出高电平（数据）
#define EPD_W21_RST_0   digitalWrite(SPI_RES,LOW)   // RST输出低电平（复位）
#define EPD_W21_RST_1   digitalWrite(SPI_RES,HIGH)  // RST输出高电平
#define isEPD_W21_BUSY  digitalRead(SPI_BUSY)       // 读取BUSY引脚状态

//==================== 屏幕参数定义 ====================
#define MONOMSB_MODE 1      // 单色高位在前模式
#define MONOLSB_MODE 2      // 单色低位在前模式
#define RED_MODE     3      // 红色模式

#define MAX_LINE_BYTES   50     // 一行字节数（400/8）
#define MAX_COLUMN_BYTES 300    // 总行数
#define ALLSCREEN_GRAGHBYTES 15000 // 全屏数据字节数（400*300/8）

//==================== 函数声明 ====================
// SPI通信相关函数
void driver_delay_us(unsigned int xus);           // 微秒级延时
void driver_delay_xms(unsigned long xms);         // 毫秒级延时
void DELAY_S(unsigned int delaytime);             // 秒级延时
void SPI_Delay(unsigned char xrate);              // SPI时序延时
void SPI_Write(unsigned char value);              // 软件SPI写一个字节
void Epaper_Write_Command(unsigned char command); // 写命令到电子纸
void Epaper_Write_Data(unsigned char command);    // 写数据到电子纸

//EPD相关
void Epaper_READBUSY(void);                       // 等待电子纸空闲
void Epaper_Spi_WriteByte(unsigned char TxData);  // SPI写字节（未用）
void Epaper_Write_Command(unsigned char cmd);     // 写命令（重复声明）
void Epaper_Write_Data(unsigned char data);       // 写数据（重复声明）

void EPD_HW_Init(void); // 电子纸初始化
void EPD_Update(void);  // 刷新显示
void EPD_DeepSleep(void); // 进入深度睡眠

// 显示相关
void EPD_WhiteScreen_ALL(const unsigned char *BW_datas,const unsigned char *R_datas); // 全屏刷新
void EPD_WhiteScreen_ALL_Clean(void); // 全屏清屏

/**
 * @brief 初始化电子纸SPI相关引脚
 * 
 * 设置 BUSY、RES、DC、CS、SCK、SPI_DIN 等引脚的输入输出模式。
 * BUSY 为输入，其余为输出。
 * 调用后可进行电子纸的SPI通信。
 */
void EPD_initSPIA() 
{
    pinMode(SPI_BUSY, INPUT);   // BUSY为输入
    pinMode(SPI_RES, OUTPUT);   // RES为输出
    pinMode(SPI_DC, OUTPUT);    // DC为输出

    pinMode(SPI_CS, OUTPUT);    // CS为输出
    pinMode(SPI_SCK, OUTPUT);   // SCK为输出
    pinMode(SPI_DIN, OUTPUT);   // SPI数据输入引脚为输出（MOSI）

    pinMode(SPI_CS_S , OUTPUT);     // 设置从设备片选为输出
    pinMode(SPI_PWR , OUTPUT);      // 设置电源控制引脚为输出

    digitalWrite(SPI_CS, HIGH);     // 片选拉高，默认不选中
    digitalWrite(SPI_CS_S, HIGH);   // 从片选拉高
    digitalWrite(SPI_PWR, HIGH);    // 电源拉高，默认上电
    digitalWrite(SPI_SCK, LOW);     // 时钟拉低，初始状态
}

//==================== 主循环 ====================
// 提示：全屏刷新时屏幕会闪烁，这是正常现象，用于消除残影。
// 若移植到其他平台，只需修改IO定义。
void Z96epd__loop() {
  while(1)
  {
    // 1. 全屏刷新显示图片
    EPD_HW_Init(); // 电子纸初始化
    EPD_WhiteScreen_ALL(gImage_BW,gImage_R); // 全屏显示图片（黑白+红色）
    EPD_DeepSleep(); // 进入深度睡眠，降低功耗
    delay(3000);   // 延时3秒

    // 2. 清屏
    EPD_HW_Init(); // 重新初始化
    EPD_WhiteScreen_ALL_Clean(); // 全屏清屏（全白）
    EPD_DeepSleep(); // 再次进入深度睡眠
    while(1); // 死循环，程序停止
  }
}


///////////////////EXTERNAL FUNCTION////////////////////////////////////////////////////////////////////////
/////////////////////delay//////////////////////////////////////
/**
 * @brief 微秒级延时函数
 * @param xus 延时的微秒数
 * @note：该函数通过空循环实现微秒级延时，适用于短时间的精确延时。
 * @example(100); // 延时100微秒
*/
void driver_delay_us(unsigned int xus)  //1us
{
  for(;xus>1;xus--);
}

/**
 * @brief 毫秒级延时函数
 * @param xms 延时的毫秒数
 * @note：该函数通过嵌套循环实现毫秒级延时，适用于较长时间的延时。
 * @example(100); // 延时100毫秒
*/
void driver_delay_xms(unsigned long xms) //1ms
{  
    unsigned long i = 0 , j=0;

    for(j=0;j<xms;j++)
  {
        for(i=0; i<256; i++);
    }
}

/**
 * @brief 秒级延时函数
 * @param delaytime 延时的秒数
 * @note：该函数通过嵌套循环实现秒级延时，适用于长时间的延时。
 */
void DELAY_S(unsigned int delaytime)     
{
  int i,j,k;
  for(i=0;i<delaytime;i++)
  {
    for(j=0;j<4000;j++)           
    {
      for(k=0;k<222;k++);
                
    }
  }
}

/////////////////////////SPI///////////////////////////////////
/**
* @brief SPI时序延时函数
* @param xrate 延时的速率，数值越大延时越长
* @note：该函数通过空循环实现SPI时序的延时，适用于控制SPI通信的节奏。
* @example SPI_Delay(1); // 短延时
*/
void SPI_Delay(unsigned char xrate)
{
  unsigned char i;
  while(xrate)
  {
    for(i=0;i<2;i++);
    xrate--;
  }
}

/**
 * @brief SPI写入一个字节数据
 * @param value 要写入的字节值
 * @note：该函数通过SPI接口将一个字节的数据写入电子纸设备。
 * @example SPI_Write(0xA5); // 写入0xA5到电子纸
 */
void SPI_Write(unsigned char value)                                    
{                                                           
    unsigned char i;  
   SPI_Delay(1);
    for(i=0; i<8; i++)   
    {
        EPD_W21_CLK_0;
       SPI_Delay(1);
       if(value & 0x80)
          EPD_W21_MOSI_1;
        else
          EPD_W21_MOSI_0;   
        value = (value << 1); 
       SPI_Delay(1);
       driver_delay_us(1);
        EPD_W21_CLK_1; 
        SPI_Delay(1);
    }
}

/**
 * @brief 写命令到电子纸
 * @param command 要写入的命令字节
 * @note：该函数通过SPI接口将一个命令字节写入电子纸设备，DC引脚设置为命令模式。
 * @example Epaper_Write_Command(0x12); // 写入复位命令
 */
void Epaper_Write_Command(unsigned char command)
{
  SPI_Delay(1);
  EPD_W21_CS_0;                   
  EPD_W21_DC_0;   // command write
  SPI_Write(command);
  EPD_W21_CS_1;
}

/**
 * @brief 写数据到电子纸
 * @param data 要写入的数据字节
 * @note：该函数通过SPI接口将一个数据字节写入电子纸设备，DC引脚设置为数据模式。
 * @example Epaper_Write_Data(0xA5); // 写入数据0xA5
 */
void Epaper_Write_Data(unsigned char command)
{
  SPI_Delay(1);
  EPD_W21_CS_0;                   
  EPD_W21_DC_1;   // command write
  SPI_Write(command);
  EPD_W21_CS_1;
}

/////////////////EPD settings Functions/////////////////////

/**
 * @brief 电子纸初始化函数，该函数执行电子纸的初始化流程，包括复位、读取硬件ID、设置寄存器等。调用后电子纸已准备好进行数据传输和显示操作。
 * @note：延时至少10ms以确保复位完成。
 * @example EPD_HW_Init(); // 初始化电子纸
 */
void EPD_HW_Init(void)
{
    EPD_W21_RST_0;  // Module reset      
    delay(1); //At least 10ms delay 
    EPD_W21_RST_1; 
    delay(1); //At least 10ms delay  
    
    // 初始化电子纸寄存器
    Epaper_READBUSY();    //waiting for the electronic paper IC to release the idle signal
    Epaper_Write_Command(0x12);     //SWRESET
    Epaper_READBUSY();  //waiting for the electronic paper IC to release the idle signal
  
    Epaper_Write_Command(0x74);
    Epaper_Write_Data(0x54);
    Epaper_Write_Command(0x7E);
    Epaper_Write_Data(0x3B);
    Epaper_Write_Command(0x2B);  // Reduce glitch under ACVCOM  
    Epaper_Write_Data(0x04);           
    Epaper_Write_Data(0x63);

    Epaper_Write_Command(0x0C);  // Soft start setting
    Epaper_Write_Data(0x8B);           
    Epaper_Write_Data(0x9C);
    Epaper_Write_Data(0x96);
    Epaper_Write_Data(0x0F);

    Epaper_Write_Command(0x01);  // Set MUX as 300
    Epaper_Write_Data(0x2B);           
    Epaper_Write_Data(0x01);
    Epaper_Write_Data(0x00);     

    Epaper_Write_Command(0x11);  // Data entry mode
    Epaper_Write_Data(0x01);         
    Epaper_Write_Command(0x44); 
    Epaper_Write_Data(0x00); // RAM x address start at 0
    Epaper_Write_Data(0x31); // RAM x address end at 31h(49+1)*8->400
    Epaper_Write_Command(0x45); 
    Epaper_Write_Data(0x2B);   // RAM y address start at 12Bh     
    Epaper_Write_Data(0x01);
    Epaper_Write_Data(0x00); // RAM y address end at 00h     
    Epaper_Write_Data(0x00);
    Epaper_Write_Command(0x3C); // board
    Epaper_Write_Data(0x01); // HIZ

    Epaper_Write_Command(0x18);
    Epaper_Write_Data(0X80);
    Epaper_Write_Command(0x22);
    Epaper_Write_Data(0XB1);  //Load Temperature and waveform setting.
    Epaper_Write_Command(0x20);
    Epaper_READBUSY();    //waiting for the electronic paper IC to release the idle signal
    

    Epaper_Write_Command(0x4E); 
    Epaper_Write_Data(0x00);
    Epaper_Write_Command(0x4F); 
    Epaper_Write_Data(0x2B);
    Epaper_Write_Data(0x01);
}
//////////////////////////////All screen update////////////////////////////////////////////
// 全屏刷新显示函数
// BW_datas: 黑白数据数组指针
// R_datas:  红色数据数组指针
/**
 * @brief 全屏刷新显示函数
 * @param BW_datas 黑白数据数组指针，包含屏幕上每个像素的黑白信息（1位/像素）
 * @param R_datas 红色数据数组指针，包含屏幕上每个像素的红色信息（1位/像素）
 * @note：该函数将黑白数据和红色数据分别写入电子纸的黑白RAM和红色RAM，并触发屏幕刷新。调用后屏幕将显示对应的图像。
 * @example EPD_WhiteScreen_ALL(gImage_BW, gImage_R); // 全屏刷新显示图像
 */
void EPD_WhiteScreen_ALL(const unsigned char *BW_datas,const unsigned char *R_datas)
{
   unsigned int i;
   // 1. 写入黑白数据到电子纸RAM
   Epaper_Write_Command(0x24);   // 0x24命令：写黑白RAM
   for(i=0;i<ALLSCREEN_GRAGHBYTES;i++)
   {
     // 逐字节写入黑白像素数据
     Epaper_Write_Data(pgm_read_byte(&BW_datas[i]));
   }

   // 2. 写入红色数据到电子纸RAM
   Epaper_Write_Command(0x26);   // 0x26命令：写红色RAM
   for(i=0;i<ALLSCREEN_GRAGHBYTES;i++)
   {
     // 逐字节写入红色像素数据，取反后写入（硬件协议要求）
     Epaper_Write_Data(~pgm_read_byte(&R_datas[i]));
   }

   // 3. 触发电子纸刷新显示
   EPD_Update();
}

/////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 刷新电子纸显示函数
 * @note：该函数通过发送特定的命令和数据触发电子纸刷新显示。调用后电子纸将根据之前写入的数据更新屏幕内容。
 * @example EPD_Update(); // 刷新电子纸显示
 */
void EPD_Update(void)
{   
  Epaper_Write_Command(0x22); //Display Update Control
  Epaper_Write_Data(0xC7);   
  Epaper_Write_Command(0x20);  //Activate Display Update Sequence
  Epaper_READBUSY();   

}

/**
 * @brief 进入电子纸深度睡眠函数
 * @note：该函数通过发送特定的命令和数据使电子纸进入深度睡眠模式，以降低功耗。调用后电子纸将进入低功耗状态，适合长时间不使用时节省电能。
 * @example EPD_DeepSleep(); // 进入深度睡眠
 */
void EPD_DeepSleep(void)
{  
  Epaper_Write_Command(0x10); //enter deep sleep
  Epaper_Write_Data(0x01); 
  delay(100);
}

/**
 * @brief 等待电子纸空闲函数
 * @note：该函数通过轮询电子纸的BUSY引脚状态来判断电子纸是否空闲。当BUSY引脚为低电平时表示电子纸空闲，可以进行下一步操作。调用后函数将阻塞直到电子纸空闲。
 * @example Epaper_READBUSY(); // 等待电子纸空闲
 */
void Epaper_READBUSY(void)
{ 
  while(1)
  {   //=1 BUSY
     if(isEPD_W21_BUSY==0) break;
  }  
}

/**
 * @brief 全屏清屏函数
 * @note：该函数将电子纸的黑白RAM写入全白数据（0xFF）和红色RAM写入全黑数据（0x00），然后触发刷新显示。调用后电子纸将显示全白屏幕。
 * @example EPD_WhiteScreen_ALL_Clean(); // 全屏清屏
 */
void EPD_WhiteScreen_ALL_Clean(void)
{
   unsigned int i;  
  Epaper_Write_Command(0x24);   //write RAM for black(0)/white (1)
   for(i=0;i<ALLSCREEN_GRAGHBYTES;i++)
   {               
     Epaper_Write_Data(0xff);
   }
  Epaper_Write_Command(0x26);   //write RAM for black(0)/white (1)
   for(i=0;i<ALLSCREEN_GRAGHBYTES;i++)
   {               
     Epaper_Write_Data(0x00);
   }
   EPD_Update();   
}



//////////////////////////////////END//////////////////////////////////////////////////

#endif