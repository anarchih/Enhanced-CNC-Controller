#ifndef __CNCCONFIG__
#define __CNCCONFIG__

#define MAX_SPINDLE_SPEED 12000

#define Y_STEP_LENGTH_MM 0.00625
#define X_STEP_LENGTH_MM 0.00625
#define Z_STEP_LENGTH_MM 0.004
#define Y_STEP_LENGTH_INCH 0.00024606299
#define X_STEP_LENGTH_INCH 0.00024606299
#define Z_STEP_LENGTH_INCH 0.00015748031

#define Y_STEP_LIMIT 200 / X_STEP_LENGTH_MM
#define X_STEP_LIMIT 170 / Y_STEP_LENGTH_MM
#define Z_STEP_LIMIT 25 / Z_STEP_LENGTH_MM

#define XInvert 0
#define YInvert 0
#define ZInvert 0

#define StepPinPort GPIOE
#define XStepPin GPIO_Pin_3
#define YStepPin GPIO_Pin_2
#define ZStepPin GPIO_Pin_4

#define DirPinPort GPIOC
#define XDirPin GPIO_Pin_8
#define YDirPin GPIO_Pin_11
#define ZDirPin GPIO_Pin_13

#define EnPinPort GPIOC
#define XEnPin GPIO_Pin_4
#define YEnPin GPIO_Pin_3
#define ZEnPin GPIO_Pin_5

#define Limit1PinPort GPIOG
#define XLimit1Pin GPIO_Pin_13
#define YLimit1Pin GPIO_Pin_9
#define ZLimit1Pin GPIO_Pin_14

#define Limit2PinPort GPIOG
#define XLimit2Pin GPIO_Pin_13
#define YLimit2Pin GPIO_Pin_9
#define ZLimit2Pin GPIO_Pin_14

#define ZCalPinPort GPIOG
#define ZCalPin GPIO_Pin_3


#endif //__CNCCONFIG__
