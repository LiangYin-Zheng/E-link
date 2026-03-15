#ifndef __EPD_SCREEN_H_
#define __EPD_SCREEN_H_

#include <Arduino.h>

typedef struct {
    uint8_t RES;
    uint8_t DC;
    uint8_t CS;
    uint8_t BUSY;
    uint8_t PWR; // optional power control pin (use 255 if unused)
} EPD_Screen;

void EPD_initPins(EPD_Screen *s);

extern EPD_Screen screen1;
extern EPD_Screen screen2;

#endif