/**
  ******************************************************************************
  * @file           : lcd_i2c.h
  * @brief          : Header for PCF8574-based I2C LCD driver
  ******************************************************************************
  */
#ifndef __LCD_I2C_H
#define __LCD_I2C_H
#include "stm32f3xx_hal.h"
/* PCF8574 I2C Address (default is often 0x27 or 0x3F, shifted left by 1) */
#define LCD_I2C_ADDR (0x27 << 1)
/* LCD Commands */
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80
/* Display Control Flags */
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00
/* Entry Mode Flags */
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTDECREMENT 0x00
/* Function Set Flags */
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_5x8DOTS 0x00
/* Backlight control */
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00
#define PIN_EN (1 << 2)  /* Enable bit */
#define PIN_RW (1 << 1)  /* Read/Write bit */
#define PIN_RS (1 << 0)  /* Register Select bit */
/* Function Prototypes */
void LCD_Init(I2C_HandleTypeDef *hi2c);
void LCD_SendCmd(char cmd);
void LCD_SendData(char data);
void LCD_SendString(char *str);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Clear(void);
#endif /* __LCD_I2C_H */
