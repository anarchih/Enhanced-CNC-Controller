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
QueueHandle_t   movementQueue;

SemaphoreHandle_t stepperXMutex;
SemaphoreHandle_t stepperYMutex;
SemaphoreHandle_t stepperZMutex;

uint32_t MovementSpeed;
uint32_t SpindleSpeed = 0;

uint32_t timer2State;

uint32_t stepperState;
int32_t xInvertCoefficient = 1;
int32_t yInvertCoefficient = 1;
int32_t zInvertCoefficient = 1;
int32_t xPos = 0;
int32_t yPos = 0;
int32_t zPos = 0;

float xDelta = 1;
float yDelta = 0;
float xErrAcc = 0;
float yErrAcc = 0;

uint32_t zOffset = 0;

static void updateSpindleSpeed(uint32_t speed){
    while(uxQueueMessagesWaiting( movementQueue )); // Clear Movements
    
    if(speed > 100)
        speed = 100;
    TIM_SetCompare1(TIM3, 2400 * speed / 100);
    return;
}

static void setStepperState(uint32_t state){
    while(uxQueueMessagesWaiting( movementQueue )); // Clear Movements

    if(state){
        GPIO_ResetBits(EnPinPort, XEnPin | YEnPin | ZEnPin);
    }else{
        GPIO_SetBits(EnPinPort, XEnPin | YEnPin | ZEnPin);
    }
    
    stepperState = state;
    return;
}

static void updateFeedrate(uint32_t feedrate){
    struct CNC_Movement_t movement;
    movement.x = 0;
    movement.y = 0;
    movement.z = 0;
    movement.speed = feedrate;
    xQueueSend( movementQueue, &movement, portMAX_DELAY );
    return;
}

void TIM2_IRQHandler(void){
    struct CNC_Movement_t movement;
    BaseType_t xTaskWokenByReceive = pdFALSE;
    uint32_t xLimitState = 1; //TODO: pulled up, can cancel
    uint32_t yLimitState = 1;
    uint32_t zLimitState = 1;

    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET){
        if(timer2State){
            if(xQueueReceiveFromISR( movementQueue, &movement, &xTaskWokenByReceive) != pdFALSE){
                if(movement.speed != -1){
                    TIM_PrescalerConfig(TIM2, 10000 / movement.speed, TIM_PSCReloadMode_Update);
                }

                xLimitState = GPIO_ReadInputDataBit(Limit1PinPort, XLimit1Pin) & GPIO_ReadInputDataBit(Limit2PinPort, XLimit2Pin);
                yLimitState = GPIO_ReadInputDataBit(Limit1PinPort, YLimit1Pin) & GPIO_ReadInputDataBit(Limit2PinPort, YLimit2Pin);
                zLimitState = GPIO_ReadInputDataBit(Limit1PinPort, ZLimit1Pin) & GPIO_ReadInputDataBit(Limit2PinPort, ZLimit2Pin);

                if(!xLimitState){
                    xPos = 0;
                }
                if(!yLimitState){
                    yPos = 0;
                }
                if(!zLimitState){
                    zPos = 0;
                }
                //TODO: Shrink This to fit 2 switch
                if(xInvertCoefficient * movement.x < 0){
                    if(!xLimitState){
                        movement.x = 0;
                    }
                    GPIO_ResetBits(DirPinPort, XDirPin);
                }else{
                    if(xPos >= X_STEP_LIMIT){
                        movement.x = 0;
                    }
                    GPIO_SetBits(DirPinPort, XDirPin);
                }

                if(yInvertCoefficient * movement.y < 0){
                    if(!yLimitState){
                        movement.y = 0;
                    }
                    GPIO_SetBits(DirPinPort, YDirPin);
                }else{
                    if(yPos >= Y_STEP_LIMIT){
                        movement.y = 0;
                    }
                    GPIO_ResetBits(DirPinPort, YDirPin);
                }

                if(zInvertCoefficient * movement.z < 0){
                    if(zPos >= Z_STEP_LIMIT){
                        movement.z = 0;
                    }
                    GPIO_SetBits(DirPinPort, ZDirPin);
                }else{
                    if(!zLimitState){
                        movement.z = 0;
                    }
                    GPIO_ResetBits(DirPinPort, ZDirPin);
                }

                if(movement.x && movement.y){
                    GPIO_SetBits(StepPinPort, XStepPin | YStepPin);
                    xPos += movement.x;
                    yPos += movement.y;
                    TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);
                }else if(movement.x){
                    GPIO_SetBits(StepPinPort, XStepPin);
                    xPos += movement.x;
                    TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);
                }else if(movement.y){
                    GPIO_SetBits(StepPinPort, YStepPin);
                    yPos += movement.y;
                    TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);
                }

                if(movement.z){
                    GPIO_SetBits(StepPinPort, ZStepPin);
                    zPos += movement.z;
                    TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);
                }

                if(xPos < 0)
                    xPos = 0;
                if(yPos < 0)
                    yPos = 0;
                if(zPos < 0)
                    zPos = 0;
            }
        }else{
            GPIO_ResetBits(StepPinPort, XStepPin | YStepPin | ZStepPin);
            TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);
        }
        timer2State = !timer2State;
        if( xTaskWokenByReceive != pdFALSE ){
            portYIELD_FROM_ISR( xTaskWokenByReceive );
        }
    }
}

void InsertMove(int32_t x, int32_t y, int32_t z){
    struct CNC_Movement_t movement;
    movement.x = x;
    movement.y = y;
    movement.z = z;
    movement.speed = -1;
    xQueueSend( movementQueue, &movement, portMAX_DELAY );
    return;
}

/* Move tool base on relative position */
/* no err = 0, else = 1*/
uint8_t moveRelativly(int32_t x, int32_t y, int32_t z){
    if(!stepperState)
        return 1;

	int32_t i, error_acc;
    int32_t error_acc_ex;
    int32_t xf, yf, zf;

	//Decide Direction of rotation
	int8_t xDirection = x < 0 ? -1 : 1;
	int8_t yDirection = y < 0 ? -1 : 1;
	int8_t zDirection = z < 0 ? -1 : 1;

	//Take Absolute Value
	x = x < 0 ? x * (-1) : x;
	y = y < 0 ? y * (-1) : y;
	z = z < 0 ? z * (-1) : z;

	if((x == y) && (x == z)){
	    for(i = 0; i < x; i++){
	      InsertMove(xDirection, yDirection, zDirection);
	    }
	}else if((x >= y) && (x >= z)){
        error_acc = x >> 1;
        error_acc_ex = x >> 1;

        for(i = 0; i < x; i++){
            error_acc -= y;
            error_acc_ex -= z;
            if(error_acc < 0){
                error_acc += x;
                yf = 1;
            }else{
                yf = 0;
            }

            if(error_acc_ex < 0){
                error_acc_ex += x;
                zf = 1;
            }else{
                zf = 0;
            }

	      InsertMove(xDirection, yf * yDirection, zf * zDirection);
        }
	}else if((y >= x) && (y >= z)){
        error_acc = y >> 1;
        error_acc_ex = y >> 1;

        for(i = 0; i < y; i++){
            error_acc -= x;
            error_acc_ex -= z;
            if(error_acc < 0){
                error_acc += y;
                xf = 1;
            }else{
                xf = 0;
            }

            if(error_acc_ex < 0){
                error_acc_ex += y;
                zf = 1;
            }else{
                zf = 0;
            }

            InsertMove(xf * xDirection, yDirection, zf * zDirection);
        }
    }else if((z >= x) && (z >= y)){
        error_acc = z >> 1;
        error_acc_ex = z >> 1;

        for(i = 0; i < z; i++){
            error_acc -= x;
            error_acc_ex -= y;
            if(error_acc < 0){
                error_acc += z;
                xf = 1;
            }else{
                xf = 0;
            }

            if(error_acc_ex < 0){
                error_acc_ex += z;
                yf = 1;
            }else{
                yf = 0;
            }

            InsertMove(xf * xDirection, yf * yDirection, zDirection);
        }
    }

	return 0;
}

static void resetHome(void){
    uint32_t flag = 1;
    uint32_t xxflag, yyflag = 0;

    updateFeedrate(800);
    while(uxQueueMessagesWaiting( movementQueue )); // Clear Movements
    while(flag){
        while(uxQueueMessagesWaiting( movementQueue )); // Clear Movements

        flag = 0;
        if(GPIO_ReadInputDataBit(Limit1PinPort, ZLimit1Pin) & GPIO_ReadInputDataBit(Limit2PinPort, ZLimit2Pin)){
            moveRelativly(0, 0, 100);
            flag = 1;
        }
    }

    flag = 1;
    xxflag = yyflag = 0;
    while(flag){
        while(uxQueueMessagesWaiting( movementQueue )); // Clear Movements

        flag = 0;
        xxflag = yyflag = 0;
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
    int32_t x1, x2, y1, y2;

    while(uxQueueMessagesWaiting( movementQueue )); // Clear Movements
    updateFeedrate(800);
    moveRelativly(20 / X_STEP_LENGTH_MM, 20 / Y_STEP_LENGTH_MM, 0);
    
    updateFeedrate(200);
    /* Probe Origin*/
    while(GPIO_ReadInputDataBit(ZCalPinPort, ZCalPin)){
        while(uxQueueMessagesWaiting( movementQueue )); // Clear Movements
        moveRelativly(0, 0, -1);
        Original++;
    }
    updateFeedrate(800);
    /*Reset Z Axis*/
    moveRelativly(0, 0, Original);
    
    /* Shift Right by 3cm*/
    moveRelativly(30 / X_STEP_LENGTH_MM, 0, 0);

    x1 = 0;
    updateFeedrate(200);
    /* Probe x = 3cm*/
    while(GPIO_ReadInputDataBit(ZCalPinPort, ZCalPin)){
        while(uxQueueMessagesWaiting( movementQueue )); // Clear Movements
        moveRelativly(0, 0, -1);
        x1++;
    }
    updateFeedrate(800);
    /*Reset Z Axis*/
    moveRelativly(0, 0, x1);

    updateFeedrate(800);
    /* Shift Right by 3cm*/
    moveRelativly(30 / X_STEP_LENGTH_MM, 0, 0);

    x2 = 0;
    updateFeedrate(200);
    /* Probe x = 6cm*/
    while(GPIO_ReadInputDataBit(ZCalPinPort, ZCalPin)){
        while(uxQueueMessagesWaiting( movementQueue )); // Clear Movements
        moveRelativly(0, 0, -1);
        x2++;
    }
    updateFeedrate(800);
    /*Reset Z Axis*/
    moveRelativly(0, 0, x2);

    xDelta = (((Original - x1) + (x1 - x2)) / 2) / (30 / X_STEP_LENGTH_MM);
    
    updateFeedrate(800);
    /* Reset X*/
    moveRelativly(-60 / X_STEP_LENGTH_MM, 0, 0);
    
    /* Start to Probe Y*/

    /* Shift Up by 3cm*/
    moveRelativly(0, 30 / Y_STEP_LENGTH_MM, 0);

    y1 = 0;
    updateFeedrate(200);
    /* Probe y = 3cm*/
    while(GPIO_ReadInputDataBit(ZCalPinPort, ZCalPin)){
        while(uxQueueMessagesWaiting( movementQueue )); // Clear Movements
        moveRelativly(0, 0, -1);
        y1++;
    }
    updateFeedrate(800);
    /*Reset Z Axis*/
    moveRelativly(0, 0, y1);

    /* Shift Up by 3cm*/
    moveRelativly(0, 30 / Y_STEP_LENGTH_MM, 0);

    y2 = 0;
    updateFeedrate(200);
    /* Probe y = 6cm*/
    while(GPIO_ReadInputDataBit(ZCalPinPort, ZCalPin)){
        while(uxQueueMessagesWaiting( movementQueue )); // Clear Movements
        moveRelativly(0, 0, -1);
        y2++;
    }
    updateFeedrate(800);
    /*Reset Z Axis*/
    moveRelativly(0, 0, y2);

    yDelta = (((Original - y1) + (y1 - y2)) / 2) / (30 / Y_STEP_LENGTH_MM);

    zOffset = Original;
    return;
}

void  CNC_controller_init(void){
    operationQueue = xQueueCreate(32, sizeof(struct CNC_Operation_t));
    movementQueue = xQueueCreate(256, sizeof(struct CNC_Movement_t));
    
    stepperXMutex = xSemaphoreCreateBinary();
    stepperYMutex = xSemaphoreCreateBinary();
    stepperZMutex = xSemaphoreCreateBinary();

    xSemaphoreGive(stepperXMutex);
    xSemaphoreGive(stepperYMutex);
    xSemaphoreGive(stepperZMutex);

    if((operationQueue == 0) || (movementQueue == 0) || (stepperXMutex == NULL) || (stepperYMutex == NULL) || (stepperZMutex == NULL)){
        while(1); //Must be initilaized
    }
    
    timer2State = 0;

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
    
    if((operationQueue == 0) || (movementQueue == 0) || (stepperXMutex == NULL) || (stepperYMutex == NULL) || (stepperZMutex == NULL)){
        return;
    }

    //updateFeedrate(800);
    //moveRelativly(0, 0, Z_STEP_LIMIT);
    //moveRelativly((-1) * X_STEP_LIMIT, 0, 0);
    //moveRelativly(0, (-1) * Y_STEP_LIMIT, 0);
    //updateFeedrate(100);
    while(1){
        xQueueReceive(operationQueue, &operation, portMAX_DELAY);

        switch(operation.opcodes){
            case moveStepper:
                moveRelativly(operation.parameter1, operation.parameter2, operation.parameter3);
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
            case homeStepper:
                resetHome();
                break;
            case homeStepperSurface:
                resetHome();
                moveRelativly(15 / X_STEP_LENGTH_MM, 15 / X_STEP_LENGTH_MM, 0);
                moveRelativly(0, 0, (-1) * zOffset);
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

    xErrAcc += x * xDelta;
    if((xErrAcc <= -1) || (xErrAcc >= 1)){
        z += (int32_t)xErrAcc;
        xErrAcc -= (int32_t)xErrAcc;
    }

    yErrAcc += y * yDelta;
    if((yErrAcc <= -1) || (yErrAcc >= 1)){
        z += (int32_t)yErrAcc;
        yErrAcc -= (int32_t)yErrAcc;
    }

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
    operation.parameter1 = 100 * speed / MAX_SPINDLE_SPEED; 
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

void CNC_HomeSurface(void){
    struct CNC_Operation_t operation;
    if(operationQueue == 0)
        return;
    operation.opcodes = homeStepperSurface;
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
