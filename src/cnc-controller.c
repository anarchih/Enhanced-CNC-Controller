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

SemaphoreHandle_t stepperXMutex;
SemaphoreHandle_t stepperYMutex;
SemaphoreHandle_t stepperZMutex;

uint32_t MovementSpeed;

uint32_t timer2State;
uint32_t timer2Count;
uint32_t timer3State;
uint32_t timer3Count;

uint32_t xStepsBuffer;
uint32_t yStepsBuffer;
uint32_t zStepsBuffer;
uint32_t xStepsHalf;
uint32_t yStepsHalf;
uint32_t zStepsHalf;

uint32_t xMaxSpeed;
uint32_t yMaxSpeed;
uint32_t zMaxSpeed;
uint32_t xRealMaxSpeed;
uint32_t yRealMaxSpeed;
uint32_t zRealMaxSpeed;
uint32_t xMinSpeed;
uint32_t yMinSpeed;
uint32_t zMinSpeed;
int32_t xCurrSpeed;
int32_t yCurrSpeed;
int32_t zCurrSpeed;

uint32_t xAccelaration;
uint32_t yAccelaration;
uint32_t zAccelaration;
uint32_t xAccelarationPosition;
uint32_t yAccelarationPosition;
uint32_t zAccelarationPosition;
uint32_t xDeAccelarationPosition;
uint32_t yDeAccelarationPosition;
uint32_t zDeAccelarationPosition;

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
	static signed portBASE_TYPE xHigherPriorityTaskWoken;
        
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)  {  
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);  
        if(timer2State){
            GPIO_SetBits(GPIOG, GPIO_Pin_13);

            xStepsBuffer--;
            
            if(++timer2Count == xCurrSpeed){
                timer2Count = 0;
                if(xStepsBuffer <= xDeAccelarationPosition){
                    if(xCurrSpeed > xAccelaration){
                        xCurrSpeed -= xAccelaration;
                        if(xCurrSpeed <= xAccelaration)
                            xCurrSpeed = xAccelaration;
                        TIM_PrescalerConfig(TIM2, 5000 / xCurrSpeed, TIM_PSCReloadMode_Update);
                    }
                }else if(xStepsBuffer >= xAccelarationPosition){
                    if(xCurrSpeed < xRealMaxSpeed){
                        xCurrSpeed += xAccelaration;
                        if(xCurrSpeed > xRealMaxSpeed)
                            xCurrSpeed = xRealMaxSpeed;
                        TIM_PrescalerConfig(TIM2, 5000 / xCurrSpeed, TIM_PSCReloadMode_Update);
                    }
                }
            }
        }else{
            GPIO_ResetBits(GPIOG, GPIO_Pin_13);
            if(xStepsBuffer <= 0){
                TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
                TIM_ClearITPendingBit(TIM2, TIM_IT_Update);  
                xSemaphoreGiveFromISR(stepperXMutex, &xHigherPriorityTaskWoken);

                if (xHigherPriorityTaskWoken) {
                    taskYIELD();
                }
            }
        }
        timer2State = !timer2State;
    }
}

void TIM3_IRQHandler(void){
	static signed portBASE_TYPE xHigherPriorityTaskWoken;
        
    if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)  {  
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  
        if(timer3State){
            GPIO_SetBits(GPIOG, GPIO_Pin_14);

            yStepsBuffer--;
            
            if(++timer3Count == yCurrSpeed){
                timer3Count = 0;
                if(yStepsBuffer <= yDeAccelarationPosition){
                    if(yCurrSpeed > yAccelaration){
                        yCurrSpeed -= yAccelaration;
                        if(yCurrSpeed <= yAccelaration)
                            yCurrSpeed = yAccelaration;
                        TIM_PrescalerConfig(TIM3, 5000 / yCurrSpeed, TIM_PSCReloadMode_Update);
                    }
                }else if(yStepsBuffer >= yAccelarationPosition){
                    if(yCurrSpeed < yRealMaxSpeed){
                        yCurrSpeed += yAccelaration;
                        if(yCurrSpeed > yRealMaxSpeed)
                            yCurrSpeed = yRealMaxSpeed;
                        TIM_PrescalerConfig(TIM3, 5000 / yCurrSpeed, TIM_PSCReloadMode_Update);
                    }
                }
            }
        }else{
            GPIO_ResetBits(GPIOG, GPIO_Pin_14);
            if(yStepsBuffer <= 0){
                TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);
                TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  
                xSemaphoreGiveFromISR(stepperYMutex, &xHigherPriorityTaskWoken);

                if (xHigherPriorityTaskWoken) {
                    taskYIELD();
                }
            }
        }
        timer3State = !timer3State;
    }
}


void  cnc_controller_init(void){
    operationQueue = xQueueCreate(256, sizeof(struct CNC_Operation_t));
    
    stepperXMutex = xSemaphoreCreateBinary();
    stepperYMutex = xSemaphoreCreateBinary();
    stepperZMutex = xSemaphoreCreateBinary();

    xSemaphoreGive(stepperXMutex);
    xSemaphoreGive(stepperYMutex);
    xSemaphoreGive(stepperZMutex);

    if((operationQueue == 0) || (stepperXMutex == NULL) || (stepperYMutex == NULL) || (stepperZMutex == NULL)){
        while(1); //Must be initilaized
    }
    
    timer2State = timer2Count = 0;

    xCurrSpeed = yCurrSpeed = zCurrSpeed = 1;
    MovementSpeed = 30;
    xStepsBuffer = yStepsBuffer = zStepsBuffer = 0;

    return;
}

void cnc_controller_depatch_task(void *pvParameters){
    struct CNC_Operation_t operation;
    float rt;
    
    if((operationQueue == 0) || (stepperXMutex == NULL) || (stepperYMutex == NULL) || (stepperZMutex == NULL)){
        return;
    }

    while(1){
        xQueueReceive(operationQueue, &operation, portMAX_DELAY);

        switch(operation.opcodes){
            case moveStepper:
                xSemaphoreTake(stepperXMutex, portMAX_DELAY);
                xSemaphoreTake(stepperYMutex, portMAX_DELAY);
                //xSemaphoreTake(stepperZMutex, portMAX_DELAY);

                xCurrSpeed = yCurrSpeed = zCurrSpeed = 1;
                
                xStepsBuffer = (operation.parameter1 > 0) ? operation.parameter1 : (-1) *  operation.parameter1;
                yStepsBuffer = (operation.parameter2 > 0) ? operation.parameter2 : (-1) *  operation.parameter2;
                
                rt = Q_rsqrt(xStepsBuffer * xStepsBuffer + yStepsBuffer * yStepsBuffer);

                xAccelaration = 10 * ((float)xStepsBuffer / (xStepsBuffer + yStepsBuffer));
                xRealMaxSpeed = MovementSpeed * xStepsBuffer * rt;
                xDeAccelarationPosition = (xRealMaxSpeed * xRealMaxSpeed / xAccelaration) / 2;
                if(xDeAccelarationPosition > (xStepsBuffer >> 1))
                    xDeAccelarationPosition = xStepsBuffer >> 1;

                xAccelarationPosition = xStepsBuffer - (xRealMaxSpeed * xRealMaxSpeed / xAccelaration) / 2;
                if(xAccelarationPosition < (xStepsBuffer >> 1))
                    xAccelarationPosition = xStepsBuffer >> 1;

                yAccelaration = 10 * ((float)yStepsBuffer / (xStepsBuffer + yStepsBuffer));
                yRealMaxSpeed = MovementSpeed * yStepsBuffer * rt;
                yDeAccelarationPosition = (yRealMaxSpeed * yRealMaxSpeed / yAccelaration) / 2;
                if(yDeAccelarationPosition > (yStepsBuffer >> 1))
                    yDeAccelarationPosition = yStepsBuffer >> 1;
                yAccelarationPosition = yStepsBuffer - (yRealMaxSpeed * yRealMaxSpeed / yAccelaration) / 2;
                if(yAccelarationPosition < (yStepsBuffer >> 1))
                    yAccelarationPosition = yStepsBuffer >> 1;

                zStepsBuffer = (operation.parameter3 > 0) ? operation.parameter3 : (-1) *  operation.parameter3;
                TIM_PrescalerConfig(TIM2, 5000 / xCurrSpeed, TIM_PSCReloadMode_Immediate);
                TIM_PrescalerConfig(TIM3, 5000 / yCurrSpeed, TIM_PSCReloadMode_Immediate);
                TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
                TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
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
