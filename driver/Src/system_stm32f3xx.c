/**
  ******************************************************************************
  * @file    system_stm32f3xx.c
  * @author  MCD Application Team
  * @brief   CMSIS System Device Source File for STM32F3xx devices.
  ******************************************************************************
  */
#include "stm32f3xx.h"
/* System Clock Variable */
uint32_t SystemCoreClock = 72000000; /* 72 MHz default clock speed */
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8]  = {0, 0, 0, 0, 1, 2, 3, 4};
/**
  * @brief  Setup the microcontroller system.
  *         Initialize the FPU setting, vector table location and Clock.
  * @param  None
  * @retval None
  */
void SystemInit(void)
{
  /* FPU settings ------------------------------------------------------------*/
  #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
  #endif
  /* Reset the RCC clock configuration to the default reset state ------------*/
  /* Set HSION bit */
  RCC->CR |= 0x00000001U;
  /* Reset CFGR register */
  RCC->CFGR &= 0xF87FC000U;
  /* Reset HSEON, CSSON and PLLON bits */
  RCC->CR &= 0xFEF6FFFFU;
  /* Reset PREDIV1[3:0] bits */
  RCC->CFGR2 &= 0xFFFFFFF0U;
  /* Reset USARTSW[1:0], I2CSW[1:0] and TIMSW bits */
  RCC->CFGR3 &= 0xF00F1100U;
  /* Disable all interrupts */
  RCC->CIR = 0x00000000U;
  /* Vector Table Relocation in Internal SRAM or FLASH */
#if defined(USER_VECT_TAB_ADDRESS)
  SCB->VTOR = VECT_TAB_BASE_ADDRESS | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM or FLASH */
#endif
}
/**
  * @brief  Update SystemCoreClock variable according to Clock Register Values.
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate(void)
{
  SystemCoreClock = 72000000;
}
