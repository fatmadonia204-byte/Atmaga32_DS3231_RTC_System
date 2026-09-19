#ifndef DS3231_INTERFACE_H
#define DS3231_INTERFACE_H

#include "../../lib/std_types.h"

typedef struct
{
    uint8 second;
    uint8 minute;
    uint8 hour;
    uint8 dayOfWeek;
    uint8 date;
    uint8 month;
    uint16 year;
} Ds3231DateTime;

void Ds3231_init(void);
uint8 Ds3231_getDateTime(Ds3231DateTime *dateTime);
uint8 Ds3231_setDateTime(const Ds3231DateTime *dateTime);
uint8 Ds3231_isOscillatorStopped(uint8 *stopped);
uint8 Ds3231_clearOscillatorStop(void);

#endif