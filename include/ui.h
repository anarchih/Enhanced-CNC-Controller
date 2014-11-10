#ifndef __JOGMODE__
#define __JOGMODE__

struct UI_Rect{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

void mainUI(void *pvParameters);
void jogUI();
 
#endif //__JOGMODE__
