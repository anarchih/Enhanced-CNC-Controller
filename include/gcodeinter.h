#ifndef GCODE_H
#define GCODE_H
void ExcuteGCode(char *gcode);

struct Exist{ 
    uint32_t x :1;
    uint32_t y :1;
    uint32_t z :1;    
    uint32_t i :1; 
    uint32_t j :1;
    uint32_t k :1;
    uint32_t f :1;    
    uint32_t s :1;
};

struct Vector{
    int op;
    float x;
    float y;
    float z;
    float f;
};
#endif
