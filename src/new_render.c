#include "new_render.h"

static uint32_t new_CurrentDrawLayer = LCD_FOREGROUND_LAYER;

void new_Init()
{
    LCD_Init();

    LCD_LayerInit();
    LCD_SetLayer(LCD_FOREGROUND_LAYER);
    LCD_SetTransparency(0x00);
}

void new_Clear(uint16_t color)
{
    uint16_t textColor, backColor;

    LCD_GetColors(&textColor, &backColor);

    LCD_SetColors(color, color);
    LCD_DrawFullRect(0, 0, LCD_PIXEL_WIDTH, LCD_PIXEL_HEIGHT);

    LCD_SetColors(textColor, backColor);
}

void new_Present()
{
    if (new_CurrentDrawLayer == LCD_FOREGROUND_LAYER) {
        LCD_SetTransparency(0xff);
        LCD_SetLayer(LCD_BACKGROUND_LAYER);
        new_CurrentDrawLayer = LCD_BACKGROUND_LAYER;

    } else if (new_CurrentDrawLayer == LCD_BACKGROUND_LAYER) {
        LCD_SetLayer(LCD_FOREGROUND_LAYER);
        LCD_SetTransparency(0x00);
        new_CurrentDrawLayer = LCD_FOREGROUND_LAYER;
    }
}

void new_DisplayStringLine(uint16_t startX, uint16_t startY, uint8_t *ptr)
{  
    uint16_t currentPosY = startY;

    while ((currentPosY < LCD_PIXEL_WIDTH) &&
            ((*ptr != 0) & (((currentPosY + LCD_GetFont()->Width) & 0xFFFF) >= LCD_GetFont()->Width))) {
        LCD_DisplayChar(startX, currentPosY, *ptr);
        currentPosY += LCD_GetFont()->Width;
        ptr++;
    }
}
