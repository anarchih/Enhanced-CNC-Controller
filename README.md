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
::
   make
   make openocdflash or make flash (Depends on what you are using)
   
Using the Software
==================
 - We Use the Touch Screen of STM32F429 DISCO to Control our machine
 
   There are modes
   - JOG mode for manual control
   - Spindle for Spindle Control (10 Steps)
   - Gcode for gcode ttransmission
  
 - Python G-Code Sender
   - Usage
  :: 
    python3 main.py -f NC_FILE -p /dev/tty*
   - Must make Control Board Enter Gcode Mode First
   
   


    
   



