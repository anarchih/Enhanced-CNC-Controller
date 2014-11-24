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
int32_t xPos = 0;
int32_t yPos = 0;
int32_t zPos = 0;

static void updateSpindleSpeed(uint32_t speed){
    while(uxQueueMessagesWaiting( movementQueue )); // Clear Movements
    
    if(speed > 100)
        speed = 100;
    TIM_SetCompare1(TIM3, 2400 * speed / 100);
    /*if(speed < 25 && speed != 0)
        speed = 25;
    
    delta = speed - SpindleSpeed;
    for(uint32_t i = 0; i < abs_int(delta) / 5.0; i++){
        TIM_SetCompare1(TIM3, 2400 * (SpindleSpeed + (delta / 5.0) * (i + 1)) / 100);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
    SpindleSpeed = speed;
    */
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

                xLimitState = GPIO_ReadInputDataBit(LimitPinPort, XLimitPin);
                yLimitState = GPIO_ReadInputDataBit(LimitPinPort, YLimitPin);
                zLimitState = GPIO_ReadInputDataBit(LimitPinPort, ZLimitPin);
                if(!xLimitState){
                    xPos = 0;
                }
                if(!yLimitState){
                    yPos = 0;
                }
                if(!zLimitState){
                    zPos = 0;
                }
                //TODO: Shrink This
                if(movement.x < 0){
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

                if(movement.y < 0){
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

                if(movement.z < 0){
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

	//Decide Direction of rotation
	int8_t xDirection = x < 0 ? -1 : 1;
	int8_t yDirection = y < 0 ? -1 : 1;

	//Take Absolute Value
	x = x < 0 ? x * (-1) : x;
	y = y < 0 ? y * (-1) : y;

	if(z > 0){
        for(i = 0; i < z; i++){
		    InsertMove(0, 0, 1);
        }
    }

	if(x == y){
	    for(i = 0; i < x; i++){
	      InsertMove(xDirection, yDirection, 0);
	    }
	}else if(x > y){
		error_acc = x / 2;
	    for(i = 0; i < x; i++){
			error_acc -= y;
			if(error_acc < 0){
				error_acc += x;
				InsertMove(xDirection, yDirection, 0);
			}else{
				InsertMove(xDirection, 0, 0);
			}
	    }
	}else{
		error_acc = y / 2;
		for(i = 0; i < y; i++){
			error_acc -= x;
			if(error_acc < 0){
				error_acc += y;
				InsertMove(xDirection, yDirection, 0);
			}else{
				InsertMove(0, yDirection, 0);
			}
		}
	}

	if(z < 0){
        for(i = z; i < 0; i++){
		    InsertMove(0, 0, -1);
        }
    }

	return 0;
}

static void resetHome(void){
    uint32_t flag = 1;

    while(uxQueueMessagesWaiting( movementQueue )); // Clear Movements
    while(flag){
        while(uxQueueMessagesWaiting( movementQueue )); // Clear Movements

        flag = 0;
        if(GPIO_ReadInputDataBit(LimitPinPort, XLimitPin)){
            moveRelativly(-100, 0, 0);
            flag = 1;
        }
        if(GPIO_ReadInputDataBit(LimitPinPort, YLimitPin)){
            moveRelativly(0, -100, 0);
            flag = 1;
        }
        if(GPIO_ReadInputDataBit(LimitPinPort, ZLimitPin)){
            moveRelativly(0, 0, 100);
            flag = 1;
        }
    }
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
