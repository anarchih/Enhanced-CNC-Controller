#ifndef __JOGMODE__
#define __JOGMODE__

struct UI_Rect{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

struct UI_Btn{
    struct UI_Rect rect;
    char* name;
};

void mainUI(void *pvParameters);
void jogUI();
 
#endif //__JOGMODE__
