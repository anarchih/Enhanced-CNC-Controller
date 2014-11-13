#ifndef NEW_WIDGET_H
#define NEW_WIDGET_H

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ioe.h"
#include "FreeRTOS.h"
#include "task.h"

#include "clib.h"

#include "new_render.h"

#define setupButton(button_, x_, y_, w_, h_, name_); \
    do { \
        button_.rect.x = x_; \
        button_.rect.y = y_; \
        button_.rect.w = w_; \
        button_.rect.h = h_; \
        button_.name = name_; \
    } while (0);

struct new_Point{
    uint32_t x;
    uint32_t y;
};

struct new_Rect{
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
};

struct new_Button {
    struct new_Rect rect;
    char* name;
};

void new_DrawButton(struct new_Button *button);

uint32_t new_PointIsInRect(struct new_Rect *rect, struct new_Point *point);

#endif /* NEw__w_IDGET_h_ */
