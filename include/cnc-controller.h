#ifndef CNC_CONTROLLER_H
#define CNC_CONTROLLER_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#define Y_STEP_LENGTH_MM 0.00625
#define X_STEP_LENGTH_MM 0.00625
#define Z_STEP_LENGTH_MM 0.004
#define Y_STEP_LENGTH_INCH 0.00024606299
#define X_STEP_LENGTH_INCH 0.00024606299
#define Z_STEP_LENGTH_INCH 0.00015748031

#define Y_STEP_LIMIT 200 / X_STEP_LENGTH_MM
#define X_STEP_LIMIT 170 / Y_STEP_LENGTH_MM
#define Z_STEP_LIMIT 25 / Z_STEP_LENGTH_MM

#define StepPinPort GPIOE
#define DirPinPort GPIOC
#define EnPinPort GPIOC
#define LimitPinPort GPIOG
#define YStepPin GPIO_Pin_2
#define XStepPin GPIO_Pin_3
#define ZStepPin GPIO_Pin_4
#define YDirPin GPIO_Pin_11
#define XDirPin GPIO_Pin_8
#define ZDirPin GPIO_Pin_13
#define YEnPin GPIO_Pin_3
#define XEnPin GPIO_Pin_4
#define ZEnPin GPIO_Pin_5
#define YLimitPin GPIO_Pin_9
#define XLimitPin GPIO_Pin_13
#define ZLimitPin GPIO_Pin_14

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
