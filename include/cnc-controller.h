#ifndef CNC_CONTROLLER_H
#define CNC_CONTROLLER_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

QueueHandle_t   operationQueue;

SemaphoreHandle_t stepperMutex;

enum CNC_Opcodes{
    moveStepper = 0,
    enableStepper,
    disableStepper,
    enableSpindle,
    disableSpindle,
};

struct CNC_Operation_t {
    uint32_t opcodes;
    void* opaque;
};

void  cnc_controller_init(void);

void cnc_controller_depatch_task(void);


#endif
