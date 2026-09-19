#ifndef CONFIG_H
#define CONFIG_H

#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#include <avr/io.h>
#include "lib/common_macros.h"

#define LCD_RS_PORT PORTA
#define LCD_RS_DDR  DDRA
#define LCD_RS_PIN  PA0

#define LCD_RW_PORT PORTA
#define LCD_RW_DDR  DDRA
#define LCD_RW_PIN  PA1

#define LCD_E_PORT  PORTA
#define LCD_E_DDR   DDRA
#define LCD_E_PIN   PA2

#define LCD_DATA_PORT PORTC
#define LCD_DATA_DDR  DDRC
#define LCD_DATA_PIN  PINC

#define LCD_DATA_D4_PIN PC7
#define LCD_DATA_D5_PIN PC6
#define LCD_DATA_D6_PIN PC5
#define LCD_DATA_D7_PIN PC4

#define BUZZER_PORT PORTD
#define BUZZER_DDR  DDRD
#define BUZZER_PIN  PD7

#define KEYPAD_PORT PORTB
#define KEYPAD_PIN  PINB
#define KEYPAD_DDR  DDRB

#define KEYPAD_COL_1_PIN PB4
#define KEYPAD_COL_2_PIN PB5
#define KEYPAD_COL_3_PIN PB6
#define KEYPAD_COL_4_PIN PB7

#define KEYPAD_ROW_4_PIN PB0
#define KEYPAD_ROW_3_PIN PB1
#define KEYPAD_ROW_2_PIN PB2
#define KEYPAD_ROW_1_PIN PB3

#define TWI_BIT_RATE 100000UL

#define APP_LCD_COLS 16U
#define APP_LCD_ROWS 2U

#define APP_MAIN_LOOP_DELAY_MS 5U
#define APP_DISPLAY_REFRESH_MS 250U

#define APP_COUNTDOWN_MAX_HOURS 99U
#define APP_STOPWATCH_MAX_HOURS 99U

#endif
