#include "FreeRTOS.h"
#include "task.h"
#include "host.h"
#include "queue.h"

#include "cnc-controller.h"
#include "stm32_f429.h"

#include "stm32f4xx_gpio.h"

#define INCLUDE_vTaskSuspend 1

QueueHandle_t   operationQueue;

SemaphoreHandle_t stepperXMutex;
SemaphoreHandle_t stepperYMutex;
SemaphoreHandle_t stepperZMutex;

uint32_t timer2State;
uint32_t timer2Count;

uint32_t xStepsBuffer;
uint32_t yStepsBuffer;
uint32_t zStepsBuffer;
uint32_t xStepsHalf;
uint32_t yStepsHalf;
uint32_t zStepsHalf;

uint32_t xMaxSpeed;
uint32_t yMaxSpeed;
uint32_t zMaxSpeed;
int32_t xCurrSpeed;
int32_t yCurrSpeed;
int32_t zCurrSpeed;

static void setStepperState(uint32_t state){
    xSemaphoreTake(stepperXMutex, 0);
    xSemaphoreTake(stepperYMutex, 0);
    xSemaphoreTake(stepperZMutex, 0);
    if(state){
        //TODO: reset GPIO 
    }else{
        //TODO: Set GPIO
    }
    xSemaphoreGive(stepperXMutex);
    xSemaphoreGive(stepperYMutex);
    xSemaphoreGive(stepperZMutex);
    return;
}

void TIM2_IRQHandler(void){
    GPIO_ToggleBits(GPIOG, GPIO_Pin_14);
    if(timer2State){
        GPIO_SetBits(GPIOG, GPIO_Pin_13);

        xStepsBuffer--;
        
        if(++timer2Count == xCurrSpeed){
            timer2Count = 0;
            if(xStepsBuffer > xStepsHalf)
                xCurrSpeed += xAccelaration;
            else
                xCurrSpeed -= xAccelaration;

            if(xCurrSpeed > xMaxSpeed)
                xCurrSpeed = xMaxSpeed;
            else if(xCurrSpeed < 0)
                xCurrSpeed = 0;

            TIM_PrescalerConfig(TIM2, 10000 / xCurrSpeed, TIM_PSCReloadMode_Immediate);
        }
    }else{
        GPIO_ResetBits(GPIOG, GPIO_Pin_13);
    }
    timer2State = !timer2State;

    if(!xStepsBuffer)
        xSemaphoreGiveFromISR(stepperXMutex, NULL);

    return;
}

void  cnc_controller_init(void){
    operationQueue = xQueueCreate(256, sizeof(struct CNC_Operation_t));
    
    stepperXMutex = xSemaphoreCreateBinary();
    stepperYMutex = xSemaphoreCreateBinary();
    stepperZMutex = xSemaphoreCreateBinary();

    if((operationQueue == 0) || (stepperXMutex == NULL) || (stepperYMutex == NULL) || (stepperZMutex == NULL)){
        while(1); //Must be initilaized
    }
    
    timer2State = timer2Count = 0;

    xCurrSpeed = yCurrSpeed = zCurrSpeed = 0;
    xMaxSpeed = yMaxSpeed = zMaxSpeed = 30;
    xStepsBuffer = yStepsBuffer = zStepsBuffer = 0;

    return;
}

void cnc_controller_depatch_task(void *pvParameters){
    struct CNC_Operation_t operation;
    
    if((operationQueue == 0) || (stepperXMutex == NULL) || (stepperYMutex == NULL) || (stepperZMutex == NULL)){
        return;
    }

    while(1){
        xQueueReceive(operationQueue, &operation, portMAX_DELAY);

        switch(operation.opcodes){
            case moveStepper:
                xSemaphoreTake(stepperXMutex, portMAX_DELAY);
                xSemaphoreTake(stepperYMutex, portMAX_DELAY);
                xSemaphoreTake(stepperZMutex, portMAX_DELAY);

                xCurrSpeed = yCurrSpeed = zCurrSpeed = 10;
                xStepsBuffer = (operation.parameter1 > 0) ? operation.parameter1 : (-1) *  operation.parameter1;
                yStepsBuffer = (operation.parameter2 > 0) ? operation.parameter2 : (-1) *  operation.parameter2;
                zStepsBuffer = (operation.parameter3 > 0) ? operation.parameter3 : (-1) *  operation.parameter3;
                xStepsHalf = xStepsBuffer >> 1;
                yStepsHalf = yStepsBuffer >> 1;
                zStepsHalf = zStepsBuffer >> 1;
                
                

                TIM_PrescalerConfig(TIM2, 10000 / xCurrSpeed, TIM_PSCReloadMode_Immediate);
                TIMER2_Enable_Interrupt();
                break;
            case enableStepper:
                setStepperState(1);
                break;
            case disableStepper:
                setStepperState(0);
                break;
            case enableSpindle:
                break;
            case disableSpindle:
                break;
        } 
    }
}

void CNC_Move(int32_t x, int32_t y, int32_t z){
    struct CNC_Operation_t operation;
    if(operationQueue == 0)
        return;

    operation.opcodes = moveStepper; 
    operation.parameter1 = x;
    operation.parameter2 = y;
    operation.parameter3 = z;

    xQueueSend(operationQueue, &operation, portMAX_DELAY);
    return;
}
