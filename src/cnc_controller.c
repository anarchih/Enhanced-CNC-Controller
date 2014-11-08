#include "FreeRTOS.h"
#include "task.h"
#include "host.h"
#include "queue.h"

#include "cnc-controller.h"
#include "stm32_f429.h"

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
    if(timer2State){
        //TODO: Set GPIO
        //

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
        }
    }else{
        //TODO: Reset GPIO
    }
    timer2State = !timer2State;

    if(!xStepsBuffer)
        xSemaphoreGiveFromISR(stepperXMutex, NULL);

    return;
}

void  cnc_controller_init(void){
    operationQueue = xQueueCreate(256, sizeof(struct CNC_Operation_t));
    
    stepperXMutex = xSemaphoreCreateMutex();
    stepperYMutex = xSemaphoreCreateMutex();
    stepperZMutex = xSemaphoreCreateMutex();

    if((operationQueue == 0) || (stepperXMutex == NULL) || (stepperYMutex == NULL) || (stepperZMutex == NULL)){
        while(1); //Must be initilaized
    }
    
    timer2State = timer2Count = 0;

    xCurrSpeed = yCurrSpeed = zCurrSpeed = 0;
    xStepsBuffer = yStepsBuffer = zStepsBuffer = 0;

    return;
}

void cnc_controller_depatch_task(void){
    struct CNC_Operation_t operation;
    
    if((operationQueue == 0) || (stepperXMutex == NULL) || (stepperYMutex == NULL) || (stepperZMutex == NULL)){
        return;
    }

    while(1){
        xQueueReceive(operationQueue, &operation, 0);

        switch(operation.opcodes){
            case moveStepper:
                xSemaphoreTake(stepperXMutex, 0);
                xSemaphoreTake(stepperYMutex, 0);
                xSemaphoreTake(stepperZMutex, 0);

                xCurrSpeed = yCurrSpeed = zCurrSpeed = 0;
                xStepsBuffer = operation.parameter1;
                yStepsBuffer = operation.parameter2;
                zStepsBuffer = operation.parameter3;
                xStepsHalf = xStepsBuffer >> 1;
                yStepsHalf = yStepsBuffer >> 1;
                zStepsHalf = zStepsBuffer >> 1;

//                TIMER2_Enable_Interrput();
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
