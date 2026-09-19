#ifndef LCD_INTERFACE_H
#define LCD_INTERFACE_H

#include "../../lib/std_types.h"

void LCD_init(void);
void LCD_clear(void);
void LCD_home(void);
void LCD_sendCommand(uint8 command);
void LCD_displayCharacter(char data);
void LCD_displayString(const char *string);
void LCD_displayStringRowColumn(uint8 row, uint8 column, const char *string);
void LCD_goToRowColumn(uint8 row, uint8 column);

#endif