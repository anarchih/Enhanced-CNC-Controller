#include "new_widget.h"

void new_DrawButton(struct new_Button *button)
{
    LCD_DrawRect(button->rect.x, button->rect.y, button->rect.h, button->rect.w);
    new_DisplayStringLine((button->rect.y + button->rect.h + 4),
            button->rect.x + ((button->rect.w - (LCD_GetFont()->Width * strlen(button->name))) / 2),
            (uint8_t*) button->name);
}

uint32_t new_PointIsInRect(struct new_Rect *rect, struct new_Point *point)
{
    if((point->x > rect->x) && \
            (point->x < rect->x + rect->w) && \
            (point->y > rect->y) && \
            (point->y < rect->y + rect->h)){
        return 1;
    }
    return 0;
}
