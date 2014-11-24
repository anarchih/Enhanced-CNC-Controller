#ifndef CNC_CONTROLLER_H
#define CNC_CONTROLLER_H

#include "CNC-CONFIG.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

enum CNC_Opcodes{
    moveStepper = 1,
    setFeedrate,
    enableStepper,
    disableStepper,
    setSpindleSpeed,
    homeStepper,
};

struct CNC_Operation_t {
    uint32_t opcodes;
    int32_t parameter1;
    int32_t parameter2;
    int32_t parameter3;
};

struct CNC_Movement_t {
    int8_t x;
    int8_t y;
    int8_t z;
    int32_t speed;
};

void  CNC_controller_init(void);

void CNC_controller_depatch_task(void *pvParameters);

void CNC_Move(int32_t x, int32_t y, int32_t z);
void CNC_SetFeedrate(uint32_t feedrate);
void CNC_EnableStepper();
void CNC_DisableStepper();
void CNC_SetSpindleSpeed();
void CNC_Home();


#endif
