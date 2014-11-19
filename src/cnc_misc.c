
#include "cnc_misc.h"

uint32_t abs_int(uint32_t num){
    return num > 0 ? num : (-1) * num;
}

float atof(const char* s){
    float rez = 0, fact = 1;
    if (*s == '-'){
        s++;
        fact = -1;
    };
    for (int point_seen = 0; *s; s++){
        if (*s == '.'){
            point_seen = 1; 
            continue;
        };
    int d = *s - '0';
    if (d >= 0 && d <= 9){
        if (point_seen) fact /= 10.0f;
        rez = rez * 10.0f + (float)d;
        };
    };
    return rez * fact;
};

float rsqrt( float number )
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

int32_t ipow(int32_t base, int32_t exp)
{
    int result = 1;
    while (exp)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;

    }

    return result;
}
