#include "FreeRTOS.h"
#include "task.h"
#include "host.h"
#include "queue.h"

#include "cnc-controller.h"

static void setStepperState(uint32_t state){
    xSemaphoreTake(stepperMutex, 0);
    if(state){
        
    }else{

    }
    xSemaphoreGive(stepperMutex);
    return;
}

void  cnc_controller_init(void){
    operationQueue = xQueueCreate(256, sizeof(struct CNC_Operation_t));
    
    stepperMutex = xSemaphoreCreateMutex();

    if((operationQueue == 0) || (stepperMutex == NULL)){
        while(1); //Must be initilaized
    }

    return;
}

void cnc_controller_depatch_task(void){
    struct CNC_Operation_t operation;
    
    if((operationQueue == 0) || (stepperMutex == NULL)){
        return;
    }

    while(1){
        xQueueReceive(operationQueue, &operation, 0);

        switch(operation.opcodes){
            case moveStepper:
                xSemaphoreTake(stepperMutex, 0);


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
