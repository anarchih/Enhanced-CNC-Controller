#include "FreeRTOS.h"
#include "task.h"
#include "host.h"
#include "queue.h"

#include "cnc-controller.h"
#include "stm32_f429.h"
#include "fio.h"
#include "clib.h"

#include "stm32f4xx_gpio.h"

#define INCLUDE_vTaskSuspend 1

QueueHandle_t   operationQueue;
QueueHandle_t   movementQueue;

SemaphoreHandle_t stepperXMutex;
SemaphoreHandle_t stepperYMutex;
SemaphoreHandle_t stepperZMutex;

uint32_t MovementSpeed;

uint32_t timer2State;

uint32_t stepperState;

float Q_rsqrt( float number )
{
    long i;
    float x2, y;
    const float threehalfs = 1.5F;
 
    x2 = number * 0.5F;
    y  = number;
    i  = * ( long * ) &y;                       // evil floating point bit level hacking
    i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
    y  = * ( float * ) &i;
    y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//      y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed
 
    return y;
}

static void setStepperState(uint32_t state){
    while(uxQueueMessagesWaiting( movementQueue )); // Clear Movements

    if(state){
        //TODO: reset GPIO 
    }else{
        //TODO: Set GPIO
    }
    
    stepperState = state;
    return;
}

void TIM2_IRQHandler(void){
    struct CNC_Movement_t movement;
    TickType_t xTaskWokenByReceive = pdFALSE;

    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET){
        TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);

        if(timer2State){
            if(xQueueReceive( movementQueue, &movement, xTaskWokenByReceive) != pdFALSE){
                if(movement.x && movement.y)
                   GPIO_SetBits(GPIOG, GPIO_Pin_13 | GPIO_Pin_14);
                else if(movement.x)
                   GPIO_SetBits(GPIOG, GPIO_Pin_13);
                else if(movement.y)
                   GPIO_SetBits(GPIOG, GPIO_Pin_14);

                if( xTaskWokenByReceive != pdFALSE ){
                    portYIELD_FROM_ISR( xTaskWokenByReceive );
                }
            }
        }else{
            GPIO_ResetBits(GPIOG, GPIO_Pin_13 | GPIO_Pin_14);
        }
        timer2State = !timer2State;
    }
}

void InsertMove(int32_t x, int32_t y, int32_t z){
    struct CNC_Movement_t movement;
    movement.x = x;
    movement.y = y;
    movement.z = z;
    xQueueSend( movementQueue, &movement, portMAX_DELAY );
    return;
}

/* Move tool base on relative position */
/* no err = 0, else = 1*/
uint8_t moveRelativly(int32_t x, int32_t y, int8_t z){
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

	return 0;
}

void  CNC_controller_init(void){
    operationQueue = xQueueCreate(256, sizeof(struct CNC_Operation_t));
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

    StepperState = 1;
    return;
}

void CNC_controller_depatch_task(void *pvParameters){
    struct CNC_Operation_t operation;
    
    if((operationQueue == 0) || (movementQueue == 0) || (stepperXMutex == NULL) || (stepperYMutex == NULL) || (stepperZMutex == NULL)){
        return;
    }

    while(1){
        xQueueReceive(operationQueue, &operation, portMAX_DELAY);

        switch(operation.opcodes){
            case moveStepper:
                moveRelativly(operation.parameter1, operation.parameter2, operation.parameter3);
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
