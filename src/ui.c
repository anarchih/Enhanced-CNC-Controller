#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ioe.h"
#include "FreeRTOS.h"
#include "task.h"

#include "clib.h"
#include "cnc-controller.h"
#include "ui.h"
#include "newDraw.h"

#define SET_BTN(BTN, X, Y, W, H, NAME); \
    do { \
        BTN.rect.x = X; \
        BTN.rect.y = Y; \
        BTN.rect.width = W; \
        BTN.rect.height = H; \
        BTN.name = NAME; \
    } while (0);

static TP_STATE *tp;
static const portTickType xDelay = (1000.0 / 20.0) / portTICK_PERIOD_MS;

int32_t CurrentSpindleSpeed = 0;

extern QueueHandle_t   operationQueue;

/* main UI */
struct UI_Btn btnJogMode;
struct UI_Btn btnSpindleSetting;

/* jog UI */
struct UI_Btn btnXForward;
struct UI_Btn btnXReverse;
struct UI_Btn btnYForward;
struct UI_Btn btnYReverse;
struct UI_Btn btnZForward;
struct UI_Btn btnZReverse;
struct UI_Btn btnXFYF;
struct UI_Btn btnXRYR;
struct UI_Btn btnXRYF;
struct UI_Btn btnXFYR;
struct UI_Btn btnFast;

/* Spindle UI */
struct UI_Btn btnSpeedUp;
struct UI_Btn btnSpeedDown;

/* Sharing UI */
struct UI_Btn btnExit;

static void init(){
    SET_BTN(btnJogMode, 25, 25, 50, 50, "JOG");
    SET_BTN(btnSpindleSetting, 125, 25, 50, 50, "Spindle");

    SET_BTN(btnXFYF, 25, 25, 50, 50, "X+Y+");
    SET_BTN(btnXFYR, 165, 25, 50, 50, "X+Y-");
    SET_BTN(btnXRYF, 25, 175, 50, 50, "X-Y+");
    SET_BTN(btnXRYR, 165, 175, 50, 50, "X-Y-");
    SET_BTN(btnXForward, 95, 25, 50, 50, "X+");
    SET_BTN(btnFast, 95, 100, 50, 50, "F / S");
    SET_BTN(btnXReverse, 95, 175, 50, 50, "X-");
    SET_BTN(btnYForward, 25, 100, 50, 50, "Y+");
    SET_BTN(btnYReverse, 165, 100, 50, 50, "Y-");
    SET_BTN(btnZForward, 25, 245, 50, 50, "Z+");
    SET_BTN(btnZReverse, 165, 245, 50, 50, "Z-");

    SET_BTN(btnSpeedUp, 95, 25, 50, 50, "SpeedUp");
    SET_BTN(btnSpeedDown, 95, 175, 50, 50, "SlowDown");

    SET_BTN(btnExit, 95, 245, 50, 50, "BACK");
}


static void drawBtn(struct UI_Btn *btn){
    LCD_DrawRect(btn->rect.x, btn->rect.y, btn->rect.height, btn->rect.width);
    new_LCD_DisplayStringLine((btn->rect.y + btn->rect.height + 4),
            btn->rect.x + ((btn->rect.width - (LCD_GetFont()->Width * strlen(btn->name))) / 2),
            (uint8_t*) btn->name);
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

static void clear()
{
    LCD_SetColors(LCD_COLOR_BLACK, LCD_COLOR_BLACK);
    LCD_DrawFullRect(0, 0, LCD_PIXEL_WIDTH, LCD_PIXEL_HEIGHT);
}

static void mainUI_handleInput()
{
    tp = IOE_TP_GetState(); 

    if( tp->TouchDetected ){
        if(isInRect(&btnJogMode.rect, tp->X, tp->Y)){
            jogUI();
        }else if(isInRect(&btnSpindleSetting.rect, tp->X, tp->Y)){
            spindleUI();
        }
    }
}

static void mainUI_render()
{
    clear();

    LCD_SetTextColor(LCD_COLOR_RED);
    drawBtn(&btnJogMode);
    drawBtn(&btnSpindleSetting);

    new_LCD_DisplayStringLine(4, 55, (uint8_t*) "SELECT MODE");

    new_Present();
}

static int jogUI_handleInput()
{
    static uint32_t settedSpeed = 200;
    static uint32_t movementAmount = 20;
    tp = IOE_TP_GetState(); 

    if( tp->TouchDetected ){
        if(isInRect(&btnXForward.rect, tp->X, tp->Y)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move(movementAmount, 0, 0);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(isInRect(&btnXReverse.rect, tp->X, tp->Y)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move((-1) * movementAmount, 0, 0);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(isInRect(&btnYForward.rect, tp->X, tp->Y)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move(0, movementAmount, 0);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(isInRect(&btnYReverse.rect, tp->X, tp->Y)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move(0, (-1) * movementAmount, 0);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(isInRect(&btnXFYF.rect, tp->X, tp->Y)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move(movementAmount, movementAmount, 0);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(isInRect(&btnXFYR.rect, tp->X, tp->Y)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move(movementAmount, (-1) * movementAmount, 0);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(isInRect(&btnXRYF.rect, tp->X, tp->Y)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move((-1) * movementAmount, movementAmount, 0);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(isInRect(&btnXRYR.rect, tp->X, tp->Y)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move((-1) * movementAmount, -movementAmount, 0);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(isInRect(&btnZForward.rect, tp->X, tp->Y)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move(0, 0, movementAmount);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(isInRect(&btnZReverse.rect, tp->X, tp->Y)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move(0, 0, (-1) * movementAmount);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(isInRect(&btnFast.rect, tp->X, tp->Y)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            if(settedSpeed == 200){
                settedSpeed = 600; 
                movementAmount = 100;
            }else{
                settedSpeed = 200; 
                movementAmount = 20;
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }else if(isInRect(&btnExit.rect, tp->X, tp->Y)){
            return 1;
        }
    }

    return 0;
}

static void jogUI_render()
{
    clear();

    LCD_SetTextColor(LCD_COLOR_RED);

    new_LCD_DisplayStringLine(4, 70, (uint8_t*) "JOG MODE");

    drawBtn(&btnXFYF);
    drawBtn(&btnXRYR);
    drawBtn(&btnXFYR);
    drawBtn(&btnXRYF);
    drawBtn(&btnFast);
    drawBtn(&btnZForward);
    drawBtn(&btnZReverse);
    drawBtn(&btnXForward);
    drawBtn(&btnXReverse);
    drawBtn(&btnYForward);
    drawBtn(&btnYReverse);
    drawBtn(&btnZForward);
    drawBtn(&btnZReverse);
    drawBtn(&btnExit);

    new_Present();
}

static void spindleUI_render()
{
    clear();

    LCD_SetTextColor(LCD_COLOR_RED);

    new_LCD_DisplayStringLine(4, 70, (uint8_t*) "SPINDLE");

    drawBtn(&btnSpeedUp);
    drawBtn(&btnSpeedDown);
    drawBtn(&btnExit);

    new_Present();
}

static int spindleUI_handleInput()
{
    tp = IOE_TP_GetState(); 

    if( tp->TouchDetected ){
        if(isInRect(&btnSpeedUp.rect, tp->X, tp->Y)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CurrentSpindleSpeed += 10;
            if(CurrentSpindleSpeed > 100)
                CurrentSpindleSpeed = 100;
            CNC_SetSpindleSpeed(CurrentSpindleSpeed);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }else if(isInRect(&btnSpeedDown.rect, tp->X, tp->Y)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CurrentSpindleSpeed -= 10;
            if(CurrentSpindleSpeed < 0)
                CurrentSpindleSpeed = 0;
            CNC_SetSpindleSpeed(CurrentSpindleSpeed);
            vTaskDelay(xDelay);
        }else if(isInRect(&btnExit.rect, tp->X, tp->Y)){
            return 1;
        }
    } 
    return 0;
}

void mainUI(void *pvParameters){
    LCD_SetFont(&Font12x12);
    init();

    while(1){
        mainUI_handleInput();
        mainUI_render();

        vTaskDelay(xDelay);
    }
}

void jogUI(void){
    uint8_t back = 0;

    while(1){
        back = jogUI_handleInput();

        jogUI_render();

        if (back)
            return;

        vTaskDelay(xDelay);
    }
}

void spindleUI(void){
    uint8_t back = 0;

    while(1){
        back = spindleUI_handleInput();

        spindleUI_render();

        if (back)
            return;

        vTaskDelay(xDelay);
    }
}
