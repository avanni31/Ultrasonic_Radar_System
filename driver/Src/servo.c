/**
  ******************************************************************************
  * @file           : servo.c
  * @brief          : MG90/SG90 PWM Servo Motor driver implementation
  ******************************************************************************
  */
#include "servo.h"
static TIM_HandleTypeDef *servo_htim;
static uint32_t servo_channel;
void Servo_Init(TIM_HandleTypeDef *htim, uint32_t channel)
{
    servo_htim = htim;
    servo_channel = channel;
    /* Start PWM output channel */
    HAL_TIM_PWM_Start(servo_htim, servo_channel);
}
void Servo_SetAngle(uint8_t angle)
{
    if (angle > 180) angle = 180;
    /* mapping angle (0 to 180) to duty cycle pulse width.
     * Timer is configured at 50Hz (20ms period).
     * With TIM clock at 1 MHz (PSC = 71), ARR = 19999 (20,000 tick period):
     * - 0.5 ms pulse (500 ticks)  -> 0 degrees
     * - 1.5 ms pulse (1500 ticks) -> 90 degrees
     * - 2.5 ms pulse (2500 ticks) -> 180 degrees
     */
    uint32_t compare_value = 500 + (((uint32_t)angle * 2000) / 180);
    
    __HAL_TIM_SET_COMPARE(servo_htim, servo_channel, compare_value);
}

