#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ioe.h"
#include "FreeRTOS.h"
#include "task.h"

#include "cnc-controller.h"
#include "ui.h"

/* main UI */
struct UI_Rect btnJogMode = {25, 25, 50, 50};

/* jog UI */
struct UI_Rect btnXForward = {25, 25, 50, 50};
struct UI_Rect btnXReverse = {165, 25, 50, 50};
struct UI_Rect btnYForward = {25, 100, 50, 50};
struct UI_Rect btnYReverse = {165, 100, 50, 50};
struct UI_Rect btnZForward = {25, 175, 50, 50};
struct UI_Rect btnZReverse = {165, 175, 50, 50};
struct UI_Rect btnExit = {165, 245, 50, 50};

static void drawRect(struct UI_Rect *rect){
    LCD_DrawRect(rect->x, rect->y, rect->height, rect->width);
    return;
}

static uint32_t isInRect(struct UI_Rect *rect, uint32_t point_x, uint32_t point_y){
    if((point_x > rect->x) && \
       (point_x < rect->x + rect->width) && \
       (point_y > rect->y) && \
       (point_y < rect->y + rect->height)){
        return 1;
    }
    return 0;
}

void mainUI(void *pvParameters){
    TP_STATE *tp;
    while(1){
        LCD_Clear(LCD_COLOR_BLACK);
        drawRect(&btnJogMode);

        tp = IOE_TP_GetState(); 

        if( tp->TouchDetected ){
            if(isInRect(&btnJogMode, tp->X, tp->Y)){
                jogUI();
            }
        }
    } 
}



void jogUI(void){
    TP_STATE *tp;

    while(1){
        LCD_Clear(LCD_COLOR_BLACK);
        drawRect(&btnXForward);
        drawRect(&btnXReverse);
        drawRect(&btnYForward);
        drawRect(&btnYReverse);
        drawRect(&btnZForward);
        drawRect(&btnZReverse);
        drawRect(&btnExit);

        tp = IOE_TP_GetState(); 

        if( tp->TouchDetected ){
            if(isInRect(&btnXForward, tp->X, tp->Y)){
                CNC_SetFeedrate(100);
                CNC_Move(20, 0, 0);
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }else if(isInRect(&btnXReverse, tp->X, tp->Y)){
                CNC_SetFeedrate(100);
                CNC_Move(-20, 0, 0);
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }else if(isInRect(&btnYForward, tp->X, tp->Y)){
                CNC_SetFeedrate(100);
                CNC_Move(0, 20, 0);
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }else if(isInRect(&btnYReverse, tp->X, tp->Y)){
                CNC_SetFeedrate(100);
                CNC_Move(0, -20, 0);
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }else if(isInRect(&btnZForward, tp->X, tp->Y)){
                CNC_SetFeedrate(100);
                CNC_Move(0, 0, 20);
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }else if(isInRect(&btnZReverse, tp->X, tp->Y)){
                CNC_SetFeedrate(100);
                CNC_Move(0, 0, -20);
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }else if(isInRect(&btnExit, tp->X, tp->Y)){
                GPIO_ToggleBits(GPIOG, GPIO_Pin_13);
                return;
            }
        }
    }
}
