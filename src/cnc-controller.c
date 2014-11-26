#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "host.h"
#include "queue.h"

#include "cnc-controller.h"
#include "stm32_f429.h"
#include "fio.h"
#include "clib.h"
#include "cnc_misc.h"

#include "stm32f4xx_gpio.h"

#define INCLUDE_vTaskSuspend 1

QueueHandle_t   operationQueue;

SemaphoreHandle_t stepperXMutex;
SemaphoreHandle_t stepperYMutex;
SemaphoreHandle_t stepperZMutex;

uint32_t MovementSpeed;
uint32_t SpindleSpeed = 0;

uint32_t timer2State;
uint32_t timer4State;
uint32_t timer5State;

uint32_t stepperState;

uint32_t stepperSpeed;
int32_t xStepsBuffer;
int32_t yStepsBuffer;
int32_t zStepsBuffer;

int32_t xInvertCoefficient = 1;
int32_t yInvertCoefficient = 1;
int32_t zInvertCoefficient = 1;
int32_t xPos = 0;
int32_t yPos = 0;
int32_t zPos = 0;

float xDelta = 0;
float yDelta = 0;
float xErrAcc = 0;
float yErrAcc = 0;

static void updateSpindleSpeed(uint32_t speed){
    xSemaphoreTake(stepperXMutex, portMAX_DELAY);
    xSemaphoreTake(stepperYMutex, portMAX_DELAY);
    xSemaphoreTake(stepperZMutex, portMAX_DELAY);
    
    if(speed > 100)
        speed = 100;
    TIM_SetCompare1(TIM3, 2400 * speed / 100);

    xSemaphoreGive(stepperXMutex);
    xSemaphoreGive(stepperYMutex);
    xSemaphoreGive(stepperZMutex);
    return;
}

static void setStepperState(uint32_t state){
    xSemaphoreTake(stepperXMutex, portMAX_DELAY);
    xSemaphoreTake(stepperYMutex, portMAX_DELAY);
    xSemaphoreTake(stepperZMutex, portMAX_DELAY);

    if(state){
        GPIO_ResetBits(EnPinPort, XEnPin | YEnPin | ZEnPin);
    }else{
        GPIO_SetBits(EnPinPort, XEnPin | YEnPin | ZEnPin);
    }
    
    stepperState = state;

    xSemaphoreGive(stepperXMutex);
    xSemaphoreGive(stepperYMutex);
    xSemaphoreGive(stepperZMutex);
    return;
}

static void updateFeedrate(uint32_t feedrate){
    stepperSpeed = feedrate;
    return;
}

void TIM2_IRQHandler(void){
    BaseType_t xTaskWokenByReceive = pdFALSE;
    uint32_t xUnLimitState = 1; //TODO: pulled up, can cancel

    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET){
        if(timer2State){
            if(xStepsBuffer != 0){

                xUnLimitState = GPIO_ReadInputDataBit(Limit1PinPort, XLimit1Pin) & GPIO_ReadInputDataBit(Limit2PinPort, XLimit2Pin);

                if(!xUnLimitState){
                    xPos = 0;
                }

                //TODO: Shrink This to fit 2 switch
                if(xInvertCoefficient * xStepsBuffer < 0){
                    GPIO_ResetBits(DirPinPort, XDirPin);
                }else{
                    GPIO_SetBits(DirPinPort, XDirPin);
                }

                if(xUnLimitState){
                    GPIO_SetBits(StepPinPort, XStepPin);
                    xPos += (xStepsBuffer > 0 ? 1 : -1);
                }
                
                if(xStepsBuffer > 0) 
                    xStepsBuffer--;
                else
                    xStepsBuffer++;

                if(xPos < 0)
                    xPos = 0;
            }else{
                TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
                xSemaphoreGiveFromISR(stepperXMutex, &xTaskWokenByReceive);

                if( xTaskWokenByReceive != pdFALSE ){
                    TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);
                    portYIELD_FROM_ISR( xTaskWokenByReceive );
                }
            }
        }else{
            GPIO_ResetBits(StepPinPort, XStepPin);
        }

        timer2State = !timer2State;
        TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);
    }
}

void TIM5_IRQHandler(void){
    BaseType_t xTaskWokenByReceive = pdFALSE;
    uint32_t yUnLimitState = 1; //TODO: pulled up, can cancel

    if (TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET){
        if(timer5State){
            if(yStepsBuffer != 0){

                yUnLimitState = GPIO_ReadInputDataBit(Limit1PinPort, YLimit1Pin) & GPIO_ReadInputDataBit(Limit2PinPort, YLimit2Pin);

                if(!yUnLimitState){
                    yPos = 0;
                }

                //TODO: Shrink This to fit 2 switch
                if(yInvertCoefficient * yStepsBuffer < 0){
                    GPIO_SetBits(DirPinPort, YDirPin);
                }else{
                    GPIO_ResetBits(DirPinPort, YDirPin);
                }

                if(yUnLimitState){
                    GPIO_SetBits(StepPinPort, YStepPin);
                    yPos += (yStepsBuffer > 0 ? 1 : -1);
                }
                
                if(yStepsBuffer > 0) 
                    yStepsBuffer--;
                else
                    yStepsBuffer++;

                if(yPos < 0)
                    yPos = 0;
            }else{
                TIM_ITConfig(TIM5, TIM_IT_Update, DISABLE);
                xSemaphoreGiveFromISR(stepperYMutex, &xTaskWokenByReceive);

                if( xTaskWokenByReceive != pdFALSE ){
                    TIM_ClearITPendingBit(TIM5, TIM_FLAG_Update);
                    portYIELD_FROM_ISR( xTaskWokenByReceive );
                }
            }
        }else{
            GPIO_ResetBits(StepPinPort, YStepPin);
        }

        timer5State = !timer5State;
        TIM_ClearITPendingBit(TIM5, TIM_FLAG_Update);

        if( xTaskWokenByReceive != pdFALSE ){
            portYIELD_FROM_ISR( xTaskWokenByReceive );
        }
    }
}

void TIM4_IRQHandler(void){
    BaseType_t xTaskWokenByReceive = pdFALSE;
    uint32_t zUnLimitState = 1; //TODO: pulled up, can cancel

    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET){
        if(timer4State){
            if(zStepsBuffer != 0){

                zUnLimitState = GPIO_ReadInputDataBit(Limit1PinPort, ZLimit1Pin) & GPIO_ReadInputDataBit(Limit2PinPort, ZLimit2Pin);

                if(!zUnLimitState){
                    zPos = 0;
                }

                //TODO: Shrink This to fit 2 switch
                if(zInvertCoefficient * zStepsBuffer < 0){
                    GPIO_SetBits(DirPinPort, ZDirPin);
                }else{
                    GPIO_ResetBits(DirPinPort, ZDirPin);
                }

                if(zUnLimitState){
                    GPIO_SetBits(StepPinPort, ZStepPin);
                    zPos += (zStepsBuffer > 0 ? 1 : -1);
                }
                
                if(zStepsBuffer > 0) 
                    zStepsBuffer--;
                else
                    zStepsBuffer++;

                if(zPos < 0)
                    zPos = 0;
            }else{
                TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE);
                xSemaphoreGiveFromISR(stepperZMutex, &xTaskWokenByReceive);

                if( xTaskWokenByReceive != pdFALSE ){
                    TIM_ClearITPendingBit(TIM4, TIM_FLAG_Update);
                    portYIELD_FROM_ISR( xTaskWokenByReceive );
                }
            }
        }else{
            GPIO_ResetBits(StepPinPort, ZStepPin);
        }

        timer4State = !timer4State;
        TIM_ClearITPendingBit(TIM4, TIM_FLAG_Update);

        if( xTaskWokenByReceive != pdFALSE ){
            portYIELD_FROM_ISR( xTaskWokenByReceive );
        }
    }
}

uint8_t moveRelativly(int32_t x, int32_t y, int32_t z){

    xSemaphoreTake(stepperXMutex, portMAX_DELAY);
    xSemaphoreTake(stepperYMutex, portMAX_DELAY);
    xSemaphoreTake(stepperZMutex, portMAX_DELAY);

    xStepsBuffer = x;
    yStepsBuffer = y;
    zStepsBuffer = z;
    
    TIM_PrescalerConfig(TIM2, 10000 / (stepperSpeed * (x / (x + y + z))), TIM_PSCReloadMode_Immediate);
    TIM_PrescalerConfig(TIM5, 10000 / (stepperSpeed * (y / (x + y + z))), TIM_PSCReloadMode_Immediate);
    TIM_PrescalerConfig(TIM4, 10000 / (stepperSpeed * (z / (x + y + z))), TIM_PSCReloadMode_Immediate);
    
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

	return 0;
}

static void resetHome(void){
    uint32_t flag = 1;
    uint32_t xxflag, yyflag = 0;

    updateFeedrate(800);
    while(flag){
        flag = 0;
        if(GPIO_ReadInputDataBit(Limit1PinPort, ZLimit1Pin) & GPIO_ReadInputDataBit(Limit2PinPort, ZLimit2Pin)){
            moveRelativly(0, 0, 100);
            flag = 1;
        }
    }

    flag = 1;
    xxflag = yyflag = 0;
    while(flag){
        flag = 0;
        if(GPIO_ReadInputDataBit(Limit1PinPort, XLimit1Pin) & GPIO_ReadInputDataBit(Limit2PinPort, XLimit2Pin)){
            xxflag = 1;
            flag = 1;
        }
        if(GPIO_ReadInputDataBit(Limit1PinPort, YLimit1Pin) & GPIO_ReadInputDataBit(Limit2PinPort, YLimit2Pin)){
            yyflag = 1;
            flag = 1;
        }
        
        moveRelativly(-100 * xxflag, -100 * yyflag, 0);
    }
    updateFeedrate(200);
    return;
}

static void calibrateZAxis(void){
    int32_t Original = 0;
    int32_t temp = 0;

    updateFeedrate(800);
    moveRelativly(20 / X_STEP_LENGTH_MM, 20 / Y_STEP_LENGTH_MM, 0);
    
    updateFeedrate(200);
    while(GPIO_ReadInputDataBit(ZCalPinPort, ZCalPin)){
        moveRelativly(0, 0, -1);
        Original++;
    }
    updateFeedrate(800);
    moveRelativly(0, 0, Original);
    
    moveRelativly(30 / X_STEP_LENGTH_MM, 0, 0);

    updateFeedrate(200);
    while(GPIO_ReadInputDataBit(ZCalPinPort, ZCalPin)){
        moveRelativly(0, 0, -1);
        temp++;
    }
    updateFeedrate(800);
    moveRelativly(0, 0, temp);

    xDelta = Original - temp;
    temp = 0;

    updateFeedrate(800);
    moveRelativly(30 / X_STEP_LENGTH_MM, 0, 0);

    updateFeedrate(200);
    while(GPIO_ReadInputDataBit(ZCalPinPort, ZCalPin)){
        moveRelativly(0, 0, -1);
        temp++;
    }
    updateFeedrate(800);
    moveRelativly(0, 0, temp);

    xDelta = ((Original - temp) + xDelta) / 2;
    temp = 0;
    
    updateFeedrate(800);
    moveRelativly(-60 / X_STEP_LENGTH_MM, 0, 0);
    

    moveRelativly(0, 30 / Y_STEP_LENGTH_MM, 0);

    updateFeedrate(200);
    while(GPIO_ReadInputDataBit(ZCalPinPort, ZCalPin)){
        moveRelativly(0, 0, -1);
        temp++;
    }
    updateFeedrate(800);
    moveRelativly(0, 0, temp);

    yDelta = Original - temp;
    temp = 0;

    moveRelativly(0, 30 / Y_STEP_LENGTH_MM, 0);

    updateFeedrate(200);
    while(GPIO_ReadInputDataBit(ZCalPinPort, ZCalPin)){
        moveRelativly(0, 0, -1);
        temp++;
    }
    updateFeedrate(800);
    moveRelativly(0, 0, temp);

    yDelta = ((Original - temp) + yDelta) / 2;

    xDelta /= (30 / X_STEP_LENGTH_MM);
    yDelta /= (30 / Y_STEP_LENGTH_MM);

    return;
}

void  CNC_controller_init(void){
    operationQueue = xQueueCreate(32, sizeof(struct CNC_Operation_t));
    
    stepperXMutex = xSemaphoreCreateBinary();
    stepperYMutex = xSemaphoreCreateBinary();
    stepperZMutex = xSemaphoreCreateBinary();
    xSemaphoreGive(stepperXMutex);
    xSemaphoreGive(stepperYMutex);
    xSemaphoreGive(stepperZMutex);

    if((operationQueue == 0) || (stepperXMutex == NULL) || (stepperYMutex == NULL) || (stepperZMutex == NULL)){
        while(1); //Must be initilaized
    }
    
    timer2State = 0;
    timer4State = 0;
    timer5State = 0;

    stepperState = 1;
    
    if(XInvert)
        xInvertCoefficient = -1;
    if(YInvert)
        yInvertCoefficient = -1;
    if(ZInvert)
        zInvertCoefficient = -1;

    //updateFeedrate(800);
    //resetHome();
    //updateFeedrate(200);
    //calibrateZAxis();

    return;
}

void CNC_controller_depatch_task(void *pvParameters){
    struct CNC_Operation_t operation;
    
    if((operationQueue == 0) || (stepperXMutex == NULL) || (stepperYMutex == NULL) || (stepperZMutex == NULL)){
        return;
    }


    while(1){

        xQueueReceive(operationQueue, &operation, portMAX_DELAY);

        switch(operation.opcodes){
            case moveStepper:
                moveRelativly(operation.parameter1, operation.parameter2, operation.parameter3);
                break;
            case homeStepper:
                resetHome();
                break;
            case setFeedrate:
                updateFeedrate(operation.parameter1);
                break;
            case enableStepper:
                setStepperState(1);
                break;
            case disableStepper:
                setStepperState(0);
                break;
            case setSpindleSpeed:
                updateSpindleSpeed(operation.parameter1);
                break;
            case calZAxis:
                resetHome();
                calibrateZAxis();
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

void CNC_SetFeedrate(uint32_t feedrate){
    struct CNC_Operation_t operation;
    if(operationQueue == 0)
        return;
    operation.opcodes = setFeedrate;
    operation.parameter1 = feedrate; //TODO : Consider using means of velocity
    xQueueSend(operationQueue, &operation, portMAX_DELAY);
    return;
}

void CNC_EnableStepper(){
    struct CNC_Operation_t operation;
    if(operationQueue == 0)
        return;
    operation.opcodes = enableStepper; 
    xQueueSend(operationQueue, &operation, portMAX_DELAY);
    return;
}

void CNC_DisableStepper(){
    struct CNC_Operation_t operation;
    if(operationQueue == 0)
        return;
    operation.opcodes = disableStepper; 
    xQueueSend(operationQueue, &operation, portMAX_DELAY);
    return;
}

void CNC_SetSpindleSpeed(uint32_t speed){
    struct CNC_Operation_t operation;
    if(operationQueue == 0)
        return;
    operation.opcodes = setSpindleSpeed;
    operation.parameter1 = speed; 
    xQueueSend(operationQueue, &operation, portMAX_DELAY);
    return;
}

void CNC_Home(void){
    struct CNC_Operation_t operation;
    if(operationQueue == 0)
        return;
    operation.opcodes = homeStepper;
    xQueueSend(operationQueue, &operation, portMAX_DELAY);
    return;
}

void CNC_CalZ(void){
    struct CNC_Operation_t operation;
    if(operationQueue == 0)
        return;
    operation.opcodes = calZAxis;
    xQueueSend(operationQueue, &operation, portMAX_DELAY);
    return;
}
