/**
  ******************************************************************************
  * @file           : lcd_i2c.c
  * @brief          : PCF8574-based I2C LCD driver implementation
  ******************************************************************************
  */
#include "lcd_i2c.h"
static I2C_HandleTypeDef *lcd_hi2c;
static uint8_t lcd_backlight_val = LCD_BACKLIGHT;
/* Sends data/command using 4-bit nibbles over PCF8574 I2C expander */
static void LCD_WriteI2C(uint8_t data)
{
    HAL_I2C_Transmit(lcd_hi2c, LCD_I2C_ADDR, &data, 1, 100);
}
static void LCD_PulseEnable(uint8_t data)
{
    LCD_WriteI2C(data | PIN_EN);  /* EN High */
    HAL_Delay(1);                 /* Pulse width >= 450ns */
    LCD_WriteI2C(data & ~PIN_EN); /* EN Low */
    HAL_Delay(1);                 /* Commands need > 37us to settle */
}
static void LCD_Write4Bits(uint8_t value, uint8_t mode)
{
    uint8_t high_nibble = value & 0xF0;
    /* Combine nibble with RS mode and backlight state */
    uint8_t data = high_nibble | mode | lcd_backlight_val;
    LCD_WriteI2C(data);
    LCD_PulseEnable(data);
}
void LCD_SendCmd(char cmd)
{
    /* Send high 4 bits first, then low 4 bits, with RS = 0 */
    LCD_Write4Bits(cmd & 0xF0, 0);
    LCD_Write4Bits((cmd << 4) & 0xF0, 0);
}
void LCD_SendData(char data)
{
    /* Send high 4 bits first, then low 4 bits, with RS = 1 */
    LCD_Write4Bits(data & 0xF0, PIN_RS);
    LCD_Write4Bits((data << 4) & 0xF0, PIN_RS);
}
void LCD_Init(I2C_HandleTypeDef *hi2c)
{
    lcd_hi2c = hi2c;
    lcd_backlight_val = LCD_BACKLIGHT;
    /* Wait for LCD power-up (at least 40ms) */
    HAL_Delay(50);
    /* Initialization sequence for 4-bit mode (HD44780 datasheet) */
    LCD_Write4Bits(0x30, 0);
    HAL_Delay(5); /* Wait > 4.1ms */
    
    LCD_Write4Bits(0x30, 0);
    HAL_Delay(1); /* Wait > 100us */
    
    LCD_Write4Bits(0x30, 0);
    HAL_Delay(1);
    
    /* Set to 4-bit mode */
    LCD_Write4Bits(0x20, 0);
    HAL_Delay(1);
    /* Function Set: 4-bit, 2 lines, 5x8 font */
    LCD_SendCmd(LCD_FUNCTIONSET | LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS);
    /* Display Control: Display On, Cursor Off, Blink Off */
    LCD_SendCmd(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF);
    /* Clear Display */
    LCD_Clear();
    /* Entry Mode Set: Increment cursor, shift display off */
    LCD_SendCmd(LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT);
}
void LCD_Clear(void)
{
    LCD_SendCmd(LCD_CLEARDISPLAY);
    HAL_Delay(2); /* Clear command takes up to 1.52ms to execute */
}
void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
    if (row > 3) row = 3; /* Limit rows for safety (e.g. 16x2 or 20x4) */
    LCD_SendCmd(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}
void LCD_SendString(char *str)
{
    while (*str)
    {
        LCD_SendData(*str++);
    }
}
