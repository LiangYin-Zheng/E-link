#ifndef __EPD_SCREEN_H_
#define __EPD_SCREEN_H_

#include <Arduino.h>

// Per-screen control pins (CS, DC, RST, BUSY, PWR). MOSI/SCK are shared.
typedef struct {
    uint8_t RES;
    uint8_t DC;
    uint8_t CS;
    uint8_t BUSY;
    uint8_t PWR; // optional power control pin (use 255 if unused)
} EPD_Screen;

// Initialize the control pins for a single screen instance.
void EPD_initPins(EPD_Screen *s);

// Example: user code should define instances like:


// If the application defines `screen1` and `screen2` in a single compilation unit,
// they can be referenced elsewhere via these extern declarations.
extern EPD_Screen screen1;
extern EPD_Screen screen2;

#endif