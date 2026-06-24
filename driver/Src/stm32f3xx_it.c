/**
  ******************************************************************************
  * @file    stm32f3xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  */
#include "main.h"
#include "stm32f3xx_it.h"
#include "ultrasonic.h"
/* External Handles */
extern TIM_HandleTypeDef htim1;
/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
}
/******************************************************************************/
/* STM32F3xx Peripheral Interrupt Handlers                                    */
/******************************************************************************/
/**
  * @brief This function handles TIM1 Capture Compare interrupt.
  */
void TIM1_CC_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim1);
}
/**
  * @brief  Input Capture callback in non-blocking mode 
  * @param  htim TIM IC handle
  * @retval None
  */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  /* Delegate handling to our ultrasonic driver */
  Ultrasonic_CaptureCallback(htim);
}

