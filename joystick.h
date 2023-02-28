// Header file for the joystick module that exposes the direction enum and a couple of functions to be used by hello.c

#ifndef JOYSTICK_H
#define JOYSTICK_H

enum direction{none, up, down, left, right, pushed};

void Joystick_initializeJoystick(void);
enum direction Joystick_checkWhichDirectionIsPressed(void);
void Joystick_cleanupJoystick(void); 

#endif