#ifndef __CNC_MISC_H__
#define __CNC_MISC_H__
#include <stdint.h>

#include "stm32f4xx.h"

uint32_t abs_int(uint32_t num);
float atof(const char* s);
float rsqrt(float number);
int32_t ipow(int32_t base, int32_t exp);

#endif
