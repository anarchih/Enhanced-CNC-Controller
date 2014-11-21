Enhanced-CNC-Controller
=======================

This is a ARM based 3 Axis CNC Software. Based on FreeRTOS.

We use the following Hardware:
 - STM32F429 DISCO Board
 - TI DRV8825 for X Y Z Stepper Driving
 - A PWM based MOSFET Spindle Control

Dependience Requirement
=======================
 - Ubuntu or MAC OSX
 - st-link or openocd
 - arm-none-eabi-gcc 4.8

Buring the Software to Board
=============
```bash
   make
   make openocdflash or make flash (Depends on what you are using)
```
Using the Software
==================
 - We Use the Touch Screen of STM32F429 DISCO to Control our machine
 
   There are modes
   - JOG mode for manual control
   - Spindle for Spindle Control (10 Steps)
   - Gcode for gcode ttransmission
  
 - Python G-Code Sender
   - Usage
  ```bash
  python3 main.py -f NC_FILE -p /dev/tty*
  ```
   - Must make Control Board Enter Gcode Mode First
   
Pin Connections
===============

PA9 -> USART
PA10 -> USART

PB4 -> PWM Spindle Ctl
PB7

PB12
PB13
PB14
PB15

PC1
PC3 -> X Motor Enable
PC4 -> Y Motor Enable
PC5 -> Z Motor Enable
PC8 -> Y Motor Direction
PC11 -> X Motor Direction
PC12 
PC13 -> Z Motor Direction

PD2
PD4
PD5
PD7

PE2 -> X Motor Step
PE3 -> Y Motor Step
PE4 -> Z Motor Step
PE5
PE6

PF6

PG2
PG3
PG9 -> X Axis - Limit
PG13 -> Y Axis - Limit
PG14 -> Z Axis - Limit


    
   



