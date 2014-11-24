#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ioe.h"
#include "FreeRTOS.h"
#include "task.h"

#include "fio.h"
#include "clib.h"

#include "cnc-controller.h"
#include "cnc_misc.h"
#include "gcodeinter.h"
#include "ui.h"
#include "new_render.h"
#include "new_widget.h"

static TP_STATE *touchPannelInfo;
struct new_Point touchPannelPoint;

static const portTickType xDelay = (1000.0 / 20.0) / portTICK_PERIOD_MS;

int32_t CurrentSpindleSpeed = 0;

extern QueueHandle_t   operationQueue;

/* main UI */
struct new_Button mainUI_jogModeButton;
struct new_Button mainUI_spindleSettingButton;
struct new_Button mainUI_gcodeShellButton;

/* jog UI */
struct new_Button jobUI_xForwardButton;
struct new_Button jogUI_xReverseButton;
struct new_Button jogUI_yForwardButton;
struct new_Button jobUI_yReverseButton;
struct new_Button jobUI_zForwardButton;
struct new_Button jogUI_zReverseButton;
struct new_Button jogUI_xFYFButton;
struct new_Button jobUI_xRYRButton;
struct new_Button jogUI_xRYFButton;
struct new_Button jogUI_xFYRButton;
struct new_Button jogUI_fastSpeedToggleButton;

/* Sharing UI */
struct new_Button share_exitButton;

/* SpindleUI */
struct new_Button spindleUI_speedUpButton;
struct new_Button spindleUI_speedDownButton;
struct new_Button spindleUI_haltButton;

int32_t spindleSpeed = 0;

static void init(){
    setupButton(mainUI_jogModeButton, 25, 25, 50, 50, "JOG");
    setupButton(mainUI_spindleSettingButton, 125, 25, 50, 50, "Spindle");
    setupButton(mainUI_gcodeShellButton, 25, 125, 50, 50, "G-Code");

    setupButton(jogUI_xFYFButton, 25, 25, 50, 50, "X+Y+");
    setupButton(jogUI_xFYRButton, 165, 25, 50, 50, "X+Y-");
    setupButton(jogUI_xRYFButton, 25, 175, 50, 50, "X-Y+");
    setupButton(jobUI_xRYRButton, 165, 175, 50, 50, "X-Y-");
    setupButton(jogUI_fastSpeedToggleButton, 95, 100, 50, 50, "F / S");
    setupButton(jobUI_xForwardButton, 165, 100, 50, 50, "X+");
    setupButton(jogUI_xReverseButton, 25, 100, 50, 50, "X-");
    setupButton(jogUI_yForwardButton, 95, 25, 50, 50, "Y+");
    setupButton(jobUI_yReverseButton, 95, 175, 50, 50, "Y-");
    setupButton(jobUI_zForwardButton, 25, 245, 50, 50, "Z+");
    setupButton(jogUI_zReverseButton, 165, 245, 50, 50, "Z-");

    setupButton(spindleUI_haltButton, 165, 245, 50, 50, "HALT");
    setupButton(spindleUI_speedUpButton, 95, 25, 50, 50, "SPEED+");
    setupButton(spindleUI_speedDownButton, 95, 175, 50, 50, "SPEED-");

    setupButton(share_exitButton, 95, 245, 50, 50, "BACK");
}

static void mainUI_handleInput()
{
    touchPannelInfo = IOE_TP_GetState(); 
    touchPannelPoint.x = touchPannelInfo->X;
    touchPannelPoint.y = touchPannelInfo->Y;

    if( touchPannelInfo->TouchDetected ){

        if(new_PointIsInRect(&mainUI_jogModeButton.rect, &touchPannelPoint)){
            jogUI();
        }else if(new_PointIsInRect(&mainUI_spindleSettingButton.rect, &touchPannelPoint)){
            spindleUI();
        }else if(new_PointIsInRect(&mainUI_gcodeShellButton.rect, &touchPannelPoint)){
            gcodeUI();
        }
    }
}

static void mainUI_render()
{
    new_Clear(LCD_COLOR_BLACK);

    LCD_SetTextColor(LCD_COLOR_RED);
    new_DrawButton(&mainUI_jogModeButton);
    new_DrawButton(&mainUI_spindleSettingButton);
    new_DrawButton(&mainUI_gcodeShellButton);

    new_DisplayStringLine(4, 55, (uint8_t*) "SELECT MODE");

    new_Present();
}

static int jogUI_handleInput()
{
    static uint32_t settedSpeed = 200;
    static uint32_t movementAmount = 20;

    touchPannelInfo = IOE_TP_GetState(); 
    touchPannelPoint.x = touchPannelInfo->X;
    touchPannelPoint.y = touchPannelInfo->Y;

    if( touchPannelInfo->TouchDetected ){
        if(new_PointIsInRect(&jobUI_xForwardButton.rect, &touchPannelPoint)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move(movementAmount, 0, 0);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(new_PointIsInRect(&jogUI_xReverseButton.rect, &touchPannelPoint)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move((-1) * movementAmount, 0, 0);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(new_PointIsInRect(&jogUI_yForwardButton.rect, &touchPannelPoint)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move(0, movementAmount, 0);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(new_PointIsInRect(&jobUI_yReverseButton.rect, &touchPannelPoint)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move(0, (-1) * movementAmount, 0);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(new_PointIsInRect(&jogUI_xFYFButton.rect, &touchPannelPoint)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move(movementAmount, movementAmount, 0);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(new_PointIsInRect(&jogUI_xFYRButton.rect, &touchPannelPoint)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move(movementAmount, (-1) * movementAmount, 0);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(new_PointIsInRect(&jogUI_xRYFButton.rect, &touchPannelPoint)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move((-1) * movementAmount, movementAmount, 0);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(new_PointIsInRect(&jobUI_xRYRButton.rect, &touchPannelPoint)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move((-1) * movementAmount, -movementAmount, 0);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(new_PointIsInRect(&jobUI_zForwardButton.rect, &touchPannelPoint)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move(0, 0, movementAmount);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(new_PointIsInRect(&jogUI_zReverseButton.rect, &touchPannelPoint)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            CNC_SetFeedrate(settedSpeed);
            CNC_Move(0, 0, (-1) * movementAmount);
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }else if(new_PointIsInRect(&jogUI_fastSpeedToggleButton.rect, &touchPannelPoint)){
            while(uxQueueMessagesWaiting( operationQueue )); // Clear Movements
            if(settedSpeed == 200){
                settedSpeed = 800; 
                movementAmount = 100;
            }else{
                settedSpeed = 200; 
                movementAmount = 20;
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }else if(new_PointIsInRect(&share_exitButton.rect, &touchPannelPoint)){
            return 1;
        }

        if(new_PointIsInRect(&share_exitButton.rect, &touchPannelPoint))
            return 1;
    }

    return 0;
}

static void jogUI_render()
{
    new_Clear(LCD_COLOR_BLACK);

    LCD_SetTextColor(LCD_COLOR_RED);

    new_DisplayStringLine(4, 70, (uint8_t*) "JOG MODE");

    new_DrawButton(&jogUI_xFYFButton);
    new_DrawButton(&jobUI_xRYRButton);
    new_DrawButton(&jogUI_xFYRButton);
    new_DrawButton(&jogUI_xRYFButton);
    new_DrawButton(&jogUI_fastSpeedToggleButton);
    new_DrawButton(&jobUI_zForwardButton);
    new_DrawButton(&jogUI_zReverseButton);
    new_DrawButton(&jobUI_xForwardButton);
    new_DrawButton(&jogUI_xReverseButton);
    new_DrawButton(&jogUI_yForwardButton);
    new_DrawButton(&jobUI_yReverseButton);
    new_DrawButton(&jobUI_zForwardButton);
    new_DrawButton(&jogUI_zReverseButton);

    new_DrawButton(&share_exitButton);

    new_Present();
}

static void spindleUI_render()
{
    new_Clear(LCD_COLOR_BLACK);

    LCD_SetTextColor(LCD_COLOR_RED);
    new_DisplayStringLine(4, 70, (uint8_t*) "SPINDLE");

    new_DrawButton(&share_exitButton);

    LCD_SetTextColor(LCD_COLOR_RED);
    LCD_DrawFullRect(24, 24, 52, 202);

    if (spindleSpeed != 100) {
        LCD_SetTextColor(LCD_COLOR_BLACK);
        LCD_DrawFullRect(25, 25, 50, (100 - spindleSpeed) * 2);
    }

    LCD_SetTextColor(LCD_COLOR_RED);
    new_DisplayStringLine(225 + 4, 25, (uint8_t*) itoa("1234567890", spindleSpeed, 10));

    new_DrawButton(&spindleUI_haltButton);
    new_DrawButton(&spindleUI_speedUpButton);
    new_DrawButton(&spindleUI_speedDownButton);

    new_Present();
}

static int spindleUI_handleInput()
{
    touchPannelInfo = IOE_TP_GetState(); 
    touchPannelPoint.x = touchPannelInfo->X;
    touchPannelPoint.y = touchPannelInfo->Y;

    if (touchPannelInfo->TouchDetected) {
        if (new_PointIsInRect(&share_exitButton.rect, &touchPannelPoint))
            return 1;

        if (new_PointIsInRect(&spindleUI_speedUpButton.rect, &touchPannelPoint)) {
            spindleSpeed += 5;
            if (spindleSpeed > 100)
                spindleSpeed = 100;

            while (uxQueueMessagesWaiting(operationQueue))
                ; // Clear Movements
            CNC_SetSpindleSpeed(spindleSpeed);
            vTaskDelay(xDelay);
        }

        if (new_PointIsInRect(&spindleUI_speedDownButton.rect, &touchPannelPoint)) {
            spindleSpeed -= 5;
            if (spindleSpeed < 0)
                spindleSpeed = 0;

            while (uxQueueMessagesWaiting(operationQueue))
                ; // Clear Movements

            CNC_SetSpindleSpeed(spindleSpeed);
            vTaskDelay(xDelay);
        }
    } 

    return 0;
}

static void gcodeUI_render()
{
    new_Clear(LCD_COLOR_BLACK);

    LCD_SetTextColor(LCD_COLOR_RED);

    new_DisplayStringLine(4, 70, (uint8_t*) "GCODE");

    new_DrawButton(&share_exitButton);

    new_Present();
}

static void gcodeUI_gcodeJob()
{
	char buf[128];

    CNC_SetFeedrate(800);
    CNC_Home();
    CNC_SetFeedrate(200);

    while (1) {
        fio_printf(1, "\rWelcome to GCode Shell\r\n");
        fio_printf(1, ">");

        fio_read(0, buf, 127);
        ExcuteGCode(buf);

        fio_printf(1, "\x06");
    }
}

static int gcodeUI_handleInput()
{
    touchPannelInfo = IOE_TP_GetState(); 
    touchPannelPoint.x = touchPannelInfo->X;
    touchPannelPoint.y = touchPannelInfo->Y;

    if (touchPannelInfo->TouchDetected) {
        if (new_PointIsInRect(&share_exitButton.rect, &touchPannelPoint))
            return 1;
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

void gcodeUI(void){
    uint8_t back = 0;
    TaskHandle_t xHandle = NULL;

    xTaskCreate(gcodeUI_gcodeJob,
            "GCODE_JOB",
            512 /* stack size */, NULL, tskIDLE_PRIORITY + 1, &xHandle);

    while(1){

        back = gcodeUI_handleInput();

        gcodeUI_render();

        if (back) {
            if (xHandle != NULL)
                vTaskDelete(xHandle);
            return;
        }

        vTaskDelay(xDelay);
    }
}
