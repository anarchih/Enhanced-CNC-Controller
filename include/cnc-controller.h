#ifndef CNC_CONTROLLER_H
#define CNC_CONTROLLER_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#define xAccelaration 10 
#define yAccelaration 10 
#define zAccelaration 10 

#define xDefaultMaxSpeed 1000
#define yDefaultMaxSpeed 1000
#define zDefaultMaxSpeed 1000


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

void cnc_controller_depatch_task(void *pvParameters);

void CNC_Move(uint32_t x, uint32_t y, uint32_t z);


#endif
