#ifndef GCODE_H
#define GCODE_H
void ExcuteGCode(char *gcode);

struct Exist{ 
    unsigned int x :1;
    unsigned int y :1;
    unsigned int z :1;    
    unsigned int i :1; 
    unsigned int j :1;
    unsigned int k :1;
    unsigned int f :1;    
    unsigned int s :1;
};

struct Vector{
    int op;
    float x;
    float y;
    float z;
    float f;
};
#endif
