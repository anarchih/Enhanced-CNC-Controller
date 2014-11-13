#include "newDraw.h"

static uint32_t new_CurrentDrawLayer = LCD_FOREGROUND_LAYER;

void new_Init()
{
    LCD_Init();

    LCD_LayerInit();
    LCD_SetLayer(LCD_FOREGROUND_LAYER);
    LCD_SetTransparency(0x00);
}

void new_Present()
{
    if (new_CurrentDrawLayer == LCD_FOREGROUND_LAYER) {
        LCD_SetTransparency(0xff);
        LCD_SetLayer(LCD_BACKGROUND_LAYER);
        //LCD_SetTransparency(0x00);
        new_CurrentDrawLayer = LCD_BACKGROUND_LAYER;
    } else if (new_CurrentDrawLayer == LCD_BACKGROUND_LAYER) {
        //LCD_SetTransparency(0xff);
        LCD_SetLayer(LCD_FOREGROUND_LAYER);
        LCD_SetTransparency(0x00);
        new_CurrentDrawLayer = LCD_FOREGROUND_LAYER;
    }
}
