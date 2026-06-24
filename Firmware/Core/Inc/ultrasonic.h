/**
  ******************************************************************************
  * @file           : ultrasonic.h
  * @brief          : Header for HC-SR04 ultrasonic sensor driver
  ******************************************************************************
  */
#ifndef __ULTRASONIC_H
#define __ULTRASONIC_H
#include "stm32f3xx_hal.h"
/* Function Prototypes */
void DWT_Init(void);
void DWT_Delay_us(uint32_t microseconds);
void Ultrasonic_Init(TIM_HandleTypeDef *htim);
void Ultrasonic_Trigger(void);
float Ultrasonic_GetDistance(void);
/* Interrupt Callback Handler to be invoked by HAL TIM Input Capture Callback */
void Ultrasonic_CaptureCallback(TIM_HandleTypeDef *htim);
#endif /* __ULTRASONIC_H */

