#ifndef NEW_DRAW_H
#define NEW_DRAW_H

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ioe.h"
#include "FreeRTOS.h"

#include "task.h"

/*
 * Keyword
 *
 * LCD_DrawRect(int x, int y, int w, int h)
 * LCD_SetLayer(uint32_t layer);
 * LCD_Clear(LCD_COLOR_BLACK);
 *
 */

void new_Init();

void new_Clear(uint16_t color);
void new_Present();

#endif /* NEW_DRAW_H */
