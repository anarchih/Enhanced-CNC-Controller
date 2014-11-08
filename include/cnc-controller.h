#ifndef CNC_CONTROLLER_H
#define CNC_CONTROLLER_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

QueueHandle_t   operationQueue;

SemaphoreHandle_t stepperMutex;

uint32_t xStepsBuffer;
uint32_t yStepsBuffer;
uint32_t zStepsBuffer;

enum CNC_Opcodes{
    moveStepper = 0,
    setSpeed,
    enableStepper,
    disableStepper,
    enableSpindle,
    disableSpindle,
};

struct CNC_Operation_t {
    uint32_t opcodes;
    uint32_t parameter1;
    uint32_t parameter2;
    uint32_t parameter3;
};

void  cnc_controller_init(void);

void cnc_controller_depatch_task(void);


#endif
