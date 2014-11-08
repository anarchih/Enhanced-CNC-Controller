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

QueueHandle_t   operationQueue;

SemaphoreHandle_t stepperXMutex;
SemaphoreHandle_t stepperYMutex;
SemaphoreHandle_t stepperZMutex;

uint32_t timer2State;
uint32_t timer2Count;

uint32_t xStepsBuffer;
uint32_t yStepsBuffer;
uint32_t zStepsBuffer;
uint32_t xStepsHalf;
uint32_t yStepsHalf;
uint32_t zStepsHalf;

uint32_t xMaxSpeed;
uint32_t yMaxSpeed;
uint32_t zMaxSpeed;
int32_t xCurrSpeed;
int32_t yCurrSpeed;
int32_t zCurrSpeed;

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
