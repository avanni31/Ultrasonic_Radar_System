/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  */
#ifndef __MAIN_H
#define __MAIN_H
#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"
/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
/* Private defines -----------------------------------------------------------*/
/* MG90 Servo PWM - TIM2_CH1 */
#define SERVO_PIN GPIO_PIN_0
#define SERVO_PORT GPIOA
/* UART - USART2 */
#define USART_TX_PIN GPIO_PIN_2
#define USART_RX_PIN GPIO_PIN_3
#define USART_PORT GPIOA
/* HC-SR04 Trig - GPIO Output */
#define TRIG_PIN GPIO_PIN_9
#define TRIG_PORT GPIOA
/* HC-SR04 Echo - TIM1_CH1 Input Capture */
#define ECHO_PIN GPIO_PIN_8
#define ECHO_PORT GPIOA
/* LCD I2C - I2C1 SCL & SDA */
#define I2C_SCL_PIN GPIO_PIN_8
#define I2C_SDA_PIN GPIO_PIN_9
#define I2C_PORT GPIOB
/* Piezo Buzzer - GPIO Output */
#define BUZZER_PIN GPIO_PIN_0
#define BUZZER_PORT GPIOB
#ifdef __cplusplus
}
#endif
#endif /* __MAIN_H */
