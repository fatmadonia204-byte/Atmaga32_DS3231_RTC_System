#include "../lcd/lcd_interface.h"

#include "../lcd/lcd_register.h"
#include "../lcd/lcd_private.h"

#include <avr/interrupt.h>
#include <util/delay.h>

#include "../../mcal/dio/dio_interface.h"
#include "../../config.h"

static void LCD_disableJtag(void)
{
    uint8 status = SREG;

    cli();
    MCUCSR |= (1U << JTD);
    MCUCSR |= (1U << JTD);
    SREG = status;
}

static void LCD_pulseEnable(void)
{
    DIO_setPinValue(DIO_PORTA_ID, LCD_E_PIN, 1U);
    _delay_us(1);
    DIO_setPinValue(DIO_PORTA_ID, LCD_E_PIN, 0U);
    _delay_us(100);
}

static void LCD_sendNibble(uint8 nibble)
{
    DIO_setPinValue(DIO_PORTC_ID, LCD_DATA_D4_PIN, (uint8)((nibble >> 0U) & 1U));
    DIO_setPinValue(DIO_PORTC_ID, LCD_DATA_D5_PIN, (uint8)((nibble >> 1U) & 1U));
    DIO_setPinValue(DIO_PORTC_ID, LCD_DATA_D6_PIN, (uint8)((nibble >> 2U) & 1U));
    DIO_setPinValue(DIO_PORTC_ID, LCD_DATA_D7_PIN, (uint8)((nibble >> 3U) & 1U));
    LCD_pulseEnable();
}

void LCD_sendCommand(uint8 command)
{
    DIO_setPinValue(DIO_PORTA_ID, LCD_RS_PIN, 0U);
    DIO_setPinValue(DIO_PORTA_ID, LCD_RW_PIN, 0U);

    LCD_sendNibble(command >> 4);
    LCD_sendNibble(command & 0x0FU);

    if (command == 0x01U || command == 0x02U)
    {
        _delay_ms(2);
    }
}

void LCD_displayCharacter(char data)
{
    DIO_setPinValue(DIO_PORTA_ID, LCD_RS_PIN, 1U);
    DIO_setPinValue(DIO_PORTA_ID, LCD_RW_PIN, 0U);

    LCD_sendNibble((uint8)data >> 4);
    LCD_sendNibble((uint8)data & 0x0FU);
}

void LCD_displayString(const char *string)
{
    while (*string != '\0')
    {
        LCD_displayCharacter(*string);
        string++;
    }
}

void LCD_goToRowColumn(uint8 row, uint8 column)
{
    uint8 address = (row == 0U) ? 0x80U : 0xC0U;

    address = (uint8)(address + column);
    LCD_sendCommand(address);
}

void LCD_displayStringRowColumn(uint8 row, uint8 column, const char *string)
{
    LCD_goToRowColumn(row, column);
    LCD_displayString(string);
}

void LCD_clear(void)
{
    LCD_sendCommand(0x01U);
}

void LCD_home(void)
{
    LCD_sendCommand(0x02U);
}

void LCD_init(void)
{
    LCD_disableJtag();
    DIO_setPinDirection(DIO_PORTA_ID, LCD_RS_PIN, 1U);
    DIO_setPinDirection(DIO_PORTA_ID, LCD_RW_PIN, 1U);
    DIO_setPinDirection(DIO_PORTA_ID, LCD_E_PIN, 1U);
    DIO_setPortDirection(DIO_PORTC_ID, 0xF0U);

    DIO_setPinValue(DIO_PORTA_ID, LCD_RS_PIN, 0U);
    DIO_setPinValue(DIO_PORTA_ID, LCD_RW_PIN, 0U);
    DIO_setPinValue(DIO_PORTA_ID, LCD_E_PIN, 0U);

    _delay_ms(40);

    LCD_sendNibble(0x03U);
    _delay_ms(5);
    LCD_sendNibble(0x03U);
    _delay_us(150);
    LCD_sendNibble(0x03U);
    _delay_us(150);
    LCD_sendNibble(0x02U);

    LCD_sendCommand(0x28U);
    LCD_sendCommand(0x0CU);
    LCD_sendCommand(0x06U);
    LCD_clear();
}