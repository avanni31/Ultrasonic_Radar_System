/**
  ******************************************************************************
  * @file           : ultrasonic.c
  * @brief          : HC-SR04 ultrasonic sensor driver implementation
  ******************************************************************************
  */
#include "ultrasonic.h"
#include "main.h"
static TIM_HandleTypeDef *ultrasonic_htim;
/* State variables for input capture */
static uint32_t val1 = 0;             /* Timer count at rising edge */
static uint32_t val2 = 0;             /* Timer count at falling edge */
static uint8_t is_first_edge = 1;     /* 1 = waiting for rising edge, 0 = waiting for falling */
static uint8_t is_captured = 0;       /* Flag indicating a reading is ready */
static uint32_t echo_pulse_width = 0; /* Echo pulse width in timer ticks */
/* Initialize DWT (Data Watchpoint and Trace) unit for high-resolution microsecond delay */
void DWT_Init(void)
{
    /* Enable TRC (Trace) unit in DEMCR register */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    /* Reset cycle counter */
    DWT->CYCCNT = 0;
    /* Enable cycle counter */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}
/* Precise microsecond delay using cycle counter */
void DWT_Delay_us(uint32_t microseconds)
{
    uint32_t start_cycles = DWT->CYCCNT;
    /* 72 MHz Clock means 72 cycles per microsecond */
    uint32_t wait_cycles = microseconds * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start_cycles) < wait_cycles);
}
void Ultrasonic_Init(TIM_HandleTypeDef *htim)
{
    ultrasonic_htim = htim;
    DWT_Init();
    /* Start Timer Input Capture in Interrupt Mode */
    HAL_TIM_IC_Start_IT(ultrasonic_htim, TIM_CHANNEL_1);
}
void Ultrasonic_Trigger(void)
{
    is_captured = 0;
    
    /* 1. Send TRIG pin HIGH */
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
    /* 2. Wait 10 microseconds */
    DWT_Delay_us(10);
    /* 3. Send TRIG pin LOW */
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);
}
float Ultrasonic_GetDistance(void)
{
    /* Wait for echo pulse measurement to complete, with a safety timeout */
    uint32_t timeout = HAL_GetTick();
    while (!is_captured)
    {
        if ((HAL_GetTick() - timeout) > 30) /* 30 ms timeout */
        {
            return 0.0f; /* Out of range or timeout */
        }
    }
    /* Convert pulse width to distance in centimeters.
     * With Timer configured for 1 MHz clock (72MHz SysClk, PSC = 71),
     * 1 timer tick = 1 microsecond.
     * Speed of sound is 0.0343 cm/us.
     * Distance = (Time * Speed of Sound) / 2
     */
    float distance = ((float)echo_pulse_width * 0.0343f) / 2.0f;
    return distance;
}
/* Interrupt callback handler to be placed inside HAL_TIM_IC_CaptureCallback in stm32f3xx_it.c */
void Ultrasonic_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        if (is_first_edge) /* Rising edge captured */
        {
            /* Read rising edge timestamp */
            val1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            is_first_edge = 0;
            /* Change polarity to capture falling edge */
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
        }
        else /* Falling edge captured */
        {
            /* Read falling edge timestamp */
            val2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            
            /* Calculate pulse width accounting for timer overflow */
            if (val2 > val1)
            {
                echo_pulse_width = val2 - val1;
            }
            else
            {
                /* __HAL_TIM_GET_AUTORELOAD returns ARR register value (e.g. 0xFFFF) */
                echo_pulse_width = (__HAL_TIM_GET_AUTORELOAD(htim) - val1) + val2;
            }
            is_first_edge = 1;
            is_captured = 1;
            /* Reset polarity to capture rising edge for next measurement cycle */
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
        }
    }
}
