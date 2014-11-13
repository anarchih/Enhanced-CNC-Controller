#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ioe.h"
#include "FreeRTOS.h"
#include "task.h"

#include "fio.h"
#include "clib.h"

#include "cnc-controller.h"
#include "gcodeinter.h"
#include "ui.h"
#include "new_render.h"

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
struct UI_Btn btnGcodeShell;

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

int32_t spindleSpeed = 0;

static void init(){
    SET_BTN(btnJogMode, 25, 25, 50, 50, "JOG");
    SET_BTN(btnSpindleSetting, 125, 25, 50, 50, "Spindle");
    SET_BTN(btnGcodeShell, 225, 25, 50, 50, "G-Code");

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
    new_DisplayStringLine((btn->rect.y + btn->rect.height + 4),
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
    new_Clear(LCD_COLOR_BLACK);

    LCD_SetTextColor(LCD_COLOR_RED);
    drawBtn(&btnJogMode);
    drawBtn(&btnSpindleSetting);

    new_DisplayStringLine(4, 55, (uint8_t*) "SELECT MODE");

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
    new_Clear(LCD_COLOR_BLACK);

    LCD_SetTextColor(LCD_COLOR_RED);

    new_DisplayStringLine(4, 70, (uint8_t*) "JOG MODE");

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
    new_Clear(LCD_COLOR_BLACK);

    LCD_SetTextColor(LCD_COLOR_RED);
    new_DisplayStringLine(4, 70, (uint8_t*) "SPINDLE");

    drawBtn(&btnExit);

    LCD_SetTextColor(LCD_COLOR_RED);
    LCD_DrawFullRect(24, 24, 52, 202);

    LCD_SetTextColor(LCD_COLOR_BLACK);
    LCD_DrawFullRect(25, 25, 50, spindleSpeed * 2);

    new_Present();
}

static int spindleUI_handleInput()
{
    tp = IOE_TP_GetState(); 

    if( tp->TouchDetected ){
//        if(isInRect(&btnSpeedUp.rect, tp->X, tp->Y)){
//            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
//            CurrentSpindleSpeed += 10;
//            if(CurrentSpindleSpeed > 100)
//                CurrentSpindleSpeed = 100;
//            CNC_SetSpindleSpeed(CurrentSpindleSpeed);
//            vTaskDelay(100 / portTICK_PERIOD_MS);
//
//        }else if(isInRect(&btnSpeedDown.rect, tp->X, tp->Y)){
//            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
//            CurrentSpindleSpeed -= 10;
//            if(CurrentSpindleSpeed < 0)
//                CurrentSpindleSpeed = 0;
//            CNC_SetSpindleSpeed(CurrentSpindleSpeed);
//            vTaskDelay(xDelay);
        if(isInRect(&btnExit.rect, tp->X, tp->Y))
            return 1;

        spindleSpeed = (tp->Y - 25) / 2;
        if (spindleSpeed < 0)
            spindleSpeed = 0;
        if (spindleSpeed > 100)
            spindleSpeed = 100;
        //}
    } 
    return 0;
}

static void gcodeUI_render()
{
    new_Clear(LCD_COLOR_BLACK);

    LCD_SetTextColor(LCD_COLOR_RED);

    new_DisplayStringLine(4, 70, (uint8_t*) "GCODE");

    drawBtn(&btnExit);

    new_Present();
}

static int gcodeUI_handleInput()
{
    uint32_t ret = 0;
	char buf[128];

	fio_printf(1, "\rWelcome to GCode Shell\r\n");
    fio_read(0, buf, 127);
    ret = ExcuteGCode(buf);
    fio_printf(1, "\x06");
    return ret;
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

void gcodeUI(void){
    uint8_t back = 0;

    while(1){
	    fio_printf(1, "\rWelcome to GCode Shell\r\n");
        fio_printf(1, ">");

        back = gcodeUI_handleInput();

        gcodeUI_render();

        if (back)
            return;

        vTaskDelay(xDelay);
    }
}
