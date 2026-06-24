/**
  ******************************************************************************
  * @file           : servo.h
  * @brief          : Header for MG90/SG90 PWM Servo Motor driver
  ******************************************************************************
  */
#ifndef __SERVO_H
#define __SERVO_H
#include "stm32f3xx_hal.h"
/* Function Prototypes */
void Servo_Init(TIM_HandleTypeDef *htim, uint32_t channel);
void Servo_SetAngle(uint8_t angle);
#endif /* __SERVO_H */

