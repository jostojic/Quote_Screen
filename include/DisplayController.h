#ifndef DISPLAY_CONTROLLER_H
#define DISPLAY_CONTROLLER_H

#define USE_GXEPD2
#ifdef USE_GXEPD2
#include <GxEPD2_BW.h>
#else
#include <GxEPD.h>
#include <GxGDEW042Z15/GxGDEW042Z15.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#endif


#include <Fonts/FreeSerifBold12pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Arduino.h>

#include "Quote.h"
#include "DisplayControllerIcons.h"

class DisplayController {
    public:
        explicit DisplayController();
        void showQuote(Quote* quote);
        void showWarning(const char* message);

    private:
        #ifdef USE_GXEPD2
        GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> *_display;
        #else
        GxEPD_Class* _display;
        #endif

        uint16_t _padding;
        uint16_t _drawAreaX;
        uint16_t _drawAreaY;
        uint16_t _drawAreaWidth;
        uint16_t _drawAreaHeight;

        const GFXfont* _quoteFont;
        const GFXfont* _boldFont;   

        void _printTextWithBreaksAtSpaces(const char* text, uint16_t maximumY, bool wrap = true);
        void _printAuothorAndTitle(const char* author, const char* title, uint16_t *printedHeight);
        uint16_t _getWidthOfString(const char* string, int16_t x=0, int16_t y=0);
        uint16_t _getHeightOfString(const char* string, int16_t x=0, int16_t y=0);

};

#endif // DISPLAY_CONTROLLER_H