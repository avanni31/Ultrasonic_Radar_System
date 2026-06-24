/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body coordinating the Ultrasonic Radar System
  ******************************************************************************
  */
#include "main.h"
#include "lcd_i2c.h"
#include "ultrasonic.h"
#include "servo.h"
#include <stdio.h>
#include <string.h>
/* Peripheral Handles */
I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim1;  /* Echo Input Capture */
TIM_HandleTypeDef htim2;  /* Servo PWM Output */
UART_HandleTypeDef huart2; /* Serial telemetry */
/* Private function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART2_UART_Init(void);
/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* MCU Configuration--------------------------------------------------------*/
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();
    /* Configure the system clock to 72 MHz */
    SystemClock_Config();
    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_TIM1_Init();
    MX_TIM2_Init();
    MX_USART2_UART_Init();
    /* Initialize Drivers */
    LCD_Init(&hi2c1);
    Servo_Init(&htim2, TIM_CHANNEL_1);
    Ultrasonic_Init(&htim1);
    /* Show Boot Screen */
    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_SendString("Ultrasonic Radar");
    LCD_SetCursor(1, 0);
    LCD_SendString("Initializing...");
    HAL_Delay(1500);
    LCD_Clear();
    /* Sweep control variables */
    uint8_t angle = 0;
    int8_t step = 2; /* 2-degree increments */
    uint32_t last_sweep_time = 0;
    char lcd_line1[17];
    char lcd_line2[17];
    char uart_buf[32];
    /* Main Loop */
    while (1)
    {
        /* Sweep and measure every 40 ms (non-blocking) */
        if (HAL_GetTick() - last_sweep_time >= 40)
        {
            last_sweep_time = HAL_GetTick();
            /* 1. Command servo to target angle */
            Servo_SetAngle(angle);
            /* 2. Wait 15ms for servo mechanical settling, then trigger measurement */
            DWT_Delay_us(15000); 
            Ultrasonic_Trigger();
            
            /* 3. Get measured distance */
            float distance = Ultrasonic_GetDistance();
            /* 4. Display telemetry data on LCD */
            sprintf(lcd_line1, "Ang:%3d  D:%3dcm", angle, (int)distance);
            LCD_SetCursor(0, 0);
            LCD_SendString(lcd_line1);
            /* 5. Handle Alert Proximity Threshold (20 cm) */
            if (distance > 0.0f && distance < 20.0f)
            {
                /* Sound Buzzer */
                HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
                
                sprintf(lcd_line2, "WARN: CLOSE!    ");
            }
            else
            {
                /* Turn Buzzer OFF */
                HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
                
                sprintf(lcd_line2, "Status: Clear   ");
            }
            LCD_SetCursor(1, 0);
            LCD_SendString(lcd_line2);
            /* 6. Transmit CSV data packet over UART: "angle,distance\r\n"
             * If out-of-range or error (distance <= 0), send "angle,0"
             */
            int send_dist = (distance > 0.0f && distance <= 400.0f) ? (int)distance : 0;
            sprintf(uart_buf, "%d,%d\r\n", angle, send_dist);
            HAL_UART_Transmit(&huart2, (uint8_t*)uart_buf, strlen(uart_buf), 100);
            /* 7. Increment scanning sweep angle */
            angle += step;
            if (angle >= 180)
            {
                angle = 180;
                step = -2; /* Reverse direction */
            }
            else if (angle <= 0)
            {
                angle = 0;
                step = 2; /* Reverse direction */
            }
        }
    }
}
/**
  * @brief System Clock Configuration
  *        System Clock source = PLL (HSE / 2 * 9) = 72 MHz
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB buses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}
/**
  * @brief I2C1 Initialization Function
  * @retval None
  */
static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x2000090E; /* Standard 100 kHz mode configuration */
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESSMODE_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALLMODE_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCHMODE_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
}
/**
  * @brief TIM1 Initialization Function (Input Capture Mode)
  * @retval None
  */
static void MX_TIM1_Init(void)
{
    TIM_IC_InitTypeDef sConfigIC = {0};
    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 71; /* SysClk 72MHz / 72 = 1MHz count rate */
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 65535;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_IC_Init(&htim1) != HAL_OK)
    {
        Error_Handler();
    }
    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 0;
    if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }
}
/**
  * @brief TIM2 Initialization Function (PWM Generation Mode)
  * @retval None
  */
static void MX_TIM2_Init(void)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 71; /* SysClk 72MHz / 72 = 1MHz count rate */
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 19999; /* 20 ms Period (50 Hz) */
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 1500; /* Start at 1.5ms pulse (90 degrees) */
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }
}
/**
  * @brief USART2 Initialization Function
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}
/**
  * @brief GPIO Initialization Function
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /*Configure GPIO pin Output Level for Trig PA9 */
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);
    /*Configure GPIO pin Output Level for Buzzer PB0 */
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
    /*Configure GPIO pin : Trig PA9 */
    GPIO_InitStruct.Pin = TRIG_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TRIG_PORT, &GPIO_InitStruct);
    /*Configure GPIO pin : Buzzer PB0 */
    GPIO_InitStruct.Pin = BUZZER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUZZER_PORT, &GPIO_InitStruct);
    /* Pin settings for TIM1 IC, TIM2 PWM, USART2, and I2C1 are automatically
     * handled by the HAL initialization functions. However, they map to:
     * - TIM1_CH1: PA8 (Alternate Function)
     * - TIM2_CH1: PA0 (Alternate Function)
     * - USART2 TX/RX: PA2/PA3 (Alternate Function)
     * - I2C1 SCL/SDA: PB8/PB9 (Alternate Function)
     */
}
/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        /* Fast toggle LED if error occurs */
    }
}

