#include "dio_interface.h"

#include <avr/io.h>

#include "dio_private.h"

static volatile uint8_t *DIO_getDDR(uint8_t PortId)
{
    switch (PortId)
    {
    case DIO_PORTA_ID: return &DDRA;
    case DIO_PORTB_ID: return &DDRB;
    case DIO_PORTC_ID: return &DDRC;
    case DIO_PORTD_ID: return &DDRD;
    default: return &DDRA;
    }
}

static volatile uint8_t *DIO_getPORT(uint8_t PortId)
{
    switch (PortId)
    {
    case DIO_PORTA_ID: return &PORTA;
    case DIO_PORTB_ID: return &PORTB;
    case DIO_PORTC_ID: return &PORTC;
    case DIO_PORTD_ID: return &PORTD;
    default: return &PORTA;
    }
}

static volatile uint8_t *DIO_getPIN(uint8_t PortId)
{
    switch (PortId)
    {
    case DIO_PORTA_ID: return &PINA;
    case DIO_PORTB_ID: return &PINB;
    case DIO_PORTC_ID: return &PINC;
    case DIO_PORTD_ID: return &PIND;
    default: return &PINA;
    }
}

void DIO_setPinDirection(uint8_t PortId, uint8_t PinId, uint8_t PinDirection)
{
    if (PinDirection != 0U)
    {
        SETBIT(*DIO_getDDR(PortId), PinId);
    }
    else
    {
        CLRBIT(*DIO_getDDR(PortId), PinId);
    }
}

void DIO_setPinValue(uint8_t PortId, uint8_t PinId, uint8_t PinValue)
{
    if (PinValue != 0U)
    {
        SETBIT(*DIO_getPORT(PortId), PinId);
    }
    else
    {
        CLRBIT(*DIO_getPORT(PortId), PinId);
    }
}

void DIO_getPinValue(uint8_t PortId, uint8_t PinId, uint8_t *PinValue)
{
    *PinValue = GETBIT(*DIO_getPIN(PortId), PinId);
}

void DIO_togglePinValue(uint8_t PortId, uint8_t PinId)
{
    TGLBIT(*DIO_getPORT(PortId), PinId);
}

void DIO_activePullUpResistance(uint8_t PortId, uint8_t PinId)
{
    SETBIT(*DIO_getPORT(PortId), PinId);
}

void DIO_setPortDirection(uint8_t PortId, uint8_t PortDirection)
{
    WRITE_REG(*DIO_getDDR(PortId), PortDirection);
}

void DIO_setPortValue(uint8_t PortId, uint8_t PortValue)
{
    WRITE_REG(*DIO_getPORT(PortId), PortValue);
}

void DIO_getPortValue(uint8_t PortId, uint8_t *PortValue)
{
    *PortValue = *DIO_getPIN(PortId);
}

void DIO_togglePortValue(uint8_t PortId)
{
    WRITE_REG(*DIO_getPORT(PortId), (uint8_t)~(*DIO_getPORT(PortId)));
}