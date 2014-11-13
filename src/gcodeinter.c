#include <string.h>
#include "cnc_misc.h"
#include "gcodeinter.h"
#include "cnc-controller.h"
#define MAX_F 100.0
#define MAX_A 1
#define LOWEST_SPEED 10
#define AVOID_ERROR 1

#define X_STEP_LENGTH_MM 0.00625
#define Y_STEP_LENGTH_MM 0.00625
#define Z_STEP_LENGTH_MM 0.004
#define X_STEP_LENGTH_INCH 0.00024606299
#define Y_STEP_LENGTH_INCH 0.00024606299
#define Z_STEP_LENGTH_INCH 0.00015748031

#define RMS_SPEED_FACTOR 0.00970180395

float X_STEP_LENGTH = X_STEP_LENGTH_MM;
float Y_STEP_LENGTH = Y_STEP_LENGTH_MM;
float Z_STEP_LENGTH = Z_STEP_LENGTH_MM;

int32_t abs_mode = 1;
float curr_x = 0;
float curr_y = 0;
float curr_z = 0;
float offset_x = 0;
float offset_y = 0;
float offset_z = 0;

float curr_v = 0;

int Q_sqrt(int a){
    //return 1/Q_sqrt(a);
    return a;
}
void AccerlationLineMove(int real_target_x, int real_target_y, int real_target_z, int f){
    
    float curr_fx = 0, curr_fy = 0, curr_fz = 0, curr_f = 0, max_f;
    int target_x, target_y, target_z;
    int dec_x, dec_y, dec_z;
    float real_target_length, dec_length, target_length;
    float ax, ay, az;
    float unit_vector_x, unit_vector_y, unit_vector_z;
     
    real_target_length = Q_sqrt(real_target_x*real_target_x + real_target_y*real_target_y + real_target_z*real_target_z);
    unit_vector_x = real_target_x / real_target_length;
    unit_vector_y = real_target_y / real_target_length;
    unit_vector_z = real_target_z / real_target_length;
    
    ax = MAX_A * unit_vector_x;
    ay = MAX_A * unit_vector_y;
    az = MAX_A * unit_vector_z;

    target_x = real_target_x - unit_vector_x * AVOID_ERROR; 
    target_y = real_target_y - unit_vector_y * AVOID_ERROR; 
    target_z = real_target_z - unit_vector_z * AVOID_ERROR;  
    target_length = Q_sqrt(target_x*target_x + target_y*target_y + target_z*target_z);

    if (f*f/MAX_A > target_length){
        max_f = Q_sqrt(target_length * MAX_A);
    }else {
        max_f = f;
    }
    
    dec_length = target_length - max_f*max_f/2/MAX_A;
    dec_x = dec_length * unit_vector_x;
    dec_y = dec_length * unit_vector_y;
    dec_z = dec_length * unit_vector_z;

    while (curr_f < max_f){
        curr_fx += ax;
        curr_fy += ay;
        curr_fz += az;
        curr_f += MAX_A;
        target_x -= (int)curr_fx;
        target_y -= (int)curr_fy;
        target_z -= (int)curr_fz;
        CNC_SetFeedrate((int)curr_f);
        CNC_Move((int)curr_fx, (int)curr_fy, (int)curr_fz);
    }
    if (max_f == f)
        CNC_Move(dec_x - target_x, dec_y - target_y, dec_z - target_z);
    while (curr_f >= LOWEST_SPEED){
        curr_fx -= ax;
        curr_fy -= ay;
        curr_fz -= az;
        curr_f += MAX_A;
        target_x -= (int)curr_fx;
        target_y -= (int)curr_fy;
        target_z -= (int)curr_fz;
        CNC_SetFeedrate(curr_f);
        CNC_Move((int)curr_fx, (int)curr_fy, (int)curr_fz);
    }
    CNC_Move(real_target_x-target_x, real_target_y, real_target_z);
}

static void CheckExist(char* gcode,struct Exist *exist){
    for (int i=0; i<strlen(gcode); i++){
        switch (gcode[i]){
            case 'X':
                exist->x = 1;
                break;
            case 'Y':
                exist->y = 1;
                break;
            case 'Z':
                exist->z = 1;
                break;
            case 'I':
                exist->i = 1;
                break;
            case 'J':
                exist->j = 1;
                break;
            case 'K':
                exist->k = 1;
                break;
            case 'F':
                exist->f = 1;
                break;
            case 'S':
                exist->s = 1;
                break;
        }
    }
}

static void retriveParameters(char gcode[], struct Exist *exist, struct Vector *v1, struct Vector *v2, uint32_t *r, uint32_t *s){
    int t = strlen(gcode);
    char tmp;

    for (int i=strlen(gcode)-1; i>=1; i--){
        if ((gcode[i]<48 || gcode[i]>57) && gcode[i]!='.' && gcode[i]!='-'){
            tmp = gcode[t];
            gcode[t] = '\0';
            if(gcode[i] == 'F' && v1 != NULL)v1->f = atof(gcode+i+1);
            else if(gcode[i] == 'Z' && v1 != NULL)v1->z = atof(gcode+i+1);
            else if(gcode[i] == 'Y' && v1 != NULL)v1->y = atof(gcode+i+1);
            else if(gcode[i] == 'X' && v1 != NULL)v1->x = atof(gcode+i+1);
            else if(gcode[i] == 'I' && v2 != NULL)v2->x = atof(gcode+i+1);
            else if(gcode[i] == 'J' && v2 != NULL)v2->y = atof(gcode+i+1);
            else if(gcode[i] == 'K' && v2 != NULL)v2->z = atof(gcode+i+1);
            else if(gcode[i] == 'R' && r != NULL)*r = atof(gcode+i+1);
            else if(gcode[i] == 'S' && s != NULL)*s = atof(gcode+i+1);
            
            gcode[t] = tmp;
            t = i;
        }
    }

    CheckExist(gcode, exist);
    return;
}

void line_move(uint32_t gnum, struct Vector v, struct Exist *exist){
    struct Vector sv;
    if (abs_mode){
        sv.x = v.x - curr_x + offset_x;    
        sv.y = v.y - curr_y + offset_y;
        sv.z = v.z - curr_z + offset_z;
    }else{
        sv.x = v.x;    
        sv.y = v.y;
        sv.z = v.z;
    } 
    if(!exist->x)sv.x = 0;
    if(!exist->y)sv.y = 0;
    if(!exist->z)sv.z = 0;

    curr_x += sv.x;
    curr_y += sv.y;
    curr_z += sv.z;
    
    if(!gnum){
        v.f = MAX_F;
    }else{
        if(!exist->f)
            v.f = curr_v;
    }
    if(v.f != curr_v){
        if(v.f > 0){
        CNC_SetFeedrate((v.f / RMS_SPEED_FACTOR) / 60);
        curr_v = v.f;
        }
    }
    if (v.f < 400)
        CNC_Move((int)(sv.x/X_STEP_LENGTH), (int)(sv.y/Y_STEP_LENGTH), (int)(sv.z/Z_STEP_LENGTH));
    else 
        AccerlationLineMove((int)(sv.x/X_STEP_LENGTH), (int)(sv.y/Y_STEP_LENGTH), (int)(sv.z/Z_STEP_LENGTH), (int)sv.f);
     
}/*
    CNC_Move((int32_t)(sv.x/X_STEP_LENGTH), (int32_t)(sv.y/Y_STEP_LENGTH), (int32_t)(sv.z/Z_STEP_LENGTH));
    //TODO: Record Error 
}
*/
static void M03(uint32_t speed, struct Exist *exist){
    CNC_SetSpindleSpeed(speed);
}

static void G92(struct Vector v, struct Exist *exist){
    if(!exist->x)v.x = curr_x;
    if(!exist->y)v.y = curr_y;
    if(!exist->z)v.z = curr_z;
    
    offset_x = v.x;
    offset_y = v.y;
    offset_z = v.z;
    return;
}

/*
void G02(char gcode[], struct Exist *exist){
    struct XYZ_Vector v;
    float vi, vj, vk, R, i_pos, j_pos, x_pos, y_pos;   
    char tmp;
    for (int i=strlen(gcode)-1; i>=1; i--){
        if ((gcode[i]<48 || gcode[i]>57) && gcode[i]!='.'){
            tmp = gcode[t];
            gcode[t] = '\0';
            printf("%s ", gcode+i+1);
            if(gcode[i] == 'F')v.f = atof(gcode+i+1);
            else if(gcode[i] == 'Z')v.z = atof(gcode+i+1);
            else if(gcode[i] == 'Y')v.y = atof(gcode+i+1);
            else if(gcode[i] == 'X')v.x = atof(gcode+i+1);
            else if(gcode[i] == 'K')vk = atof(gcode+i+1);
            else if(gcode[i] == 'J')vj = atof(gcode+i+1);
            else if(gcode[i] == 'I')vi = atof(gcode+i+1);
            else if(gcode[i] == 'R')R = atof(gcode+i+1);
            gcode[t] = tmp;
            t = i;
        }
    }
    
    if (abs_mode){
        v.x = v.x - curr_x;    
        v.y = v.y - curr_y;
        v.z = v.z - curr_z;
    }
    
    if(!exist->x)v.x = 0;
    if(!exist->y)v.y = 0;
    if(!exist->z)v.z = 0; 
    if(!exist->i)vi = 0;
    if(!exist->j)vj = 0;
    if(!exist->k)vk = 0;
    if(!exist->f)v.f = 0;
    float circle_x, circle_y, circle_z;
    float e;
    float new_R = (R+e)/2;
    x_pos = (x_pos/2);
    if (circle_x == 0)
    for (float theta = theta1; theta<theta2; theta+=0.1)
        
}*/

uint32_t ExcuteGCode(char *gcode){
    struct Exist exist;
    struct Vector v1;
    uint32_t s;

    // G00 G01 G02 G03 G90 G91 G92 M02 M03 M04 M17 M18 
    if (strncmp(gcode, "G00", 3) == 0 || 
        strncmp(gcode, "G0 ", 3) == 0  ){ 

        retriveParameters(gcode, &exist, &v1, NULL, NULL, NULL);
        line_move(0, v1, &exist);
    }else if( strncmp(gcode, "G01", 3) == 0 ||
        strncmp(gcode, "G1", 2) == 0  ){

        retriveParameters(gcode, &exist, &v1, NULL, NULL, NULL);
        line_move(0, v1, &exist);
    }else if (strncmp(gcode, "G04", 3) == 0){

    }else if (strncmp(gcode, "G20", 3) == 0){
        X_STEP_LENGTH = X_STEP_LENGTH_INCH;
        Y_STEP_LENGTH = Y_STEP_LENGTH_INCH;
        Z_STEP_LENGTH = Z_STEP_LENGTH_INCH;
    }else if (strncmp(gcode, "G21", 3) == 0){
        X_STEP_LENGTH = X_STEP_LENGTH_MM;
        Y_STEP_LENGTH = Y_STEP_LENGTH_MM;
        Z_STEP_LENGTH = Z_STEP_LENGTH_MM;
    }else if (strncmp(gcode, "G90", 3) == 0){
        abs_mode = 1;
    }else if (strncmp(gcode, "G91", 3) == 0){
        abs_mode = 0;
    }else if (strncmp(gcode, "G92", 3) == 0){
        retriveParameters(gcode, &exist, &v1, NULL, NULL, NULL);
        G92(v1, &exist);
    }else if (strncmp(gcode, "M02", 3) == 0){
        return 1;
    }else if (strncmp(gcode, "M03", 3) == 0){
        retriveParameters(gcode, &exist, NULL, NULL, NULL, &s);
        M03(s, &exist);
    }else if (strncmp(gcode, "M05", 3) == 0){
        CNC_SetSpindleSpeed(0);
    }else if (strncmp(gcode, "M17", 3) == 0){
        CNC_EnableStepper();
    }else if (strncmp(gcode, "M18", 3) == 0){
        CNC_DisableStepper();
    }
    return 0;
}


/*
int main(){

    char gcode[] = "G00 X123.1 Y231.2 F100";
    ExcuteGCode(gcode);
    return 0;
}*/
