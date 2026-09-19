#ifndef DIO_INTERFACE_H
#define DIO_INTERFACE_H

#include <stdint.h>

#define DIO_PORTA_ID 0U
#define DIO_PORTB_ID 1U
#define DIO_PORTC_ID 2U
#define DIO_PORTD_ID 3U

void DIO_setPinDirection(uint8_t PortId, uint8_t PinId, uint8_t PinDirection);
void DIO_setPinValue(uint8_t PortId, uint8_t PinId, uint8_t PinValue);
void DIO_getPinValue(uint8_t PortId, uint8_t PinId, uint8_t *PinValue);
void DIO_togglePinValue(uint8_t PortId, uint8_t PinId);
void DIO_activePullUpResistance(uint8_t PortId, uint8_t PinId);
void DIO_setPortDirection(uint8_t PortId, uint8_t PortDirection);
void DIO_setPortValue(uint8_t PortId, uint8_t PortValue);
void DIO_getPortValue(uint8_t PortId, uint8_t *PortValue);
void DIO_togglePortValue(uint8_t PortId);

#endif