#include "../rtc/ds3231_interface.h"

#include "../rtc/ds3231_register.h"
#include "../rtc/ds3231_private.h"

#include "../../mcal/twi/twi_interface.h"
#include "../../config.h"

static uint8 Ds3231_bcdToDecimal(uint8 value)
{
    return (uint8)(((value >> 4) * 10U) + (value & 0x0FU));
}

static uint8 Ds3231_decimalToBcd(uint8 value)
{
    return (uint8)(((value / 10U) << 4) | (value % 10U));
}

void Ds3231_init(void)
{
    TWI_init(TWI_BIT_RATE);
}

uint8 Ds3231_getDateTime(Ds3231DateTime *dateTime)
{
    uint8 buffer[7];
    uint8 index;

    if (TWI_start(DS3231_ADDRESS_WRITE) == 0U)
    {
        return 0U;
    }

    if (TWI_write(DS3231_REG_SECONDS) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    if (TWI_start(DS3231_ADDRESS_READ) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    for (index = 0U; index < 6U; index++)
    {
        if (TWI_readAck(&buffer[index]) == 0U)
        {
            TWI_stop();
            return 0U;
        }
    }

    if (TWI_readNack(&buffer[6]) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    TWI_stop();

    dateTime->second = Ds3231_bcdToDecimal((uint8)(buffer[0] & 0x7FU));
    dateTime->minute = Ds3231_bcdToDecimal((uint8)(buffer[1] & 0x7FU));

    if ((buffer[2] & 0x40U) != 0U)
    {
        uint8 hour = Ds3231_bcdToDecimal((uint8)(buffer[2] & 0x1FU));
        uint8 pm = (uint8)((buffer[2] & 0x20U) != 0U);

        if (hour == 12U)
        {
            dateTime->hour = pm ? 12U : 0U;
        }
        else
        {
            dateTime->hour = pm ? (uint8)(hour + 12U) : hour;
        }
    }
    else
    {
        dateTime->hour = Ds3231_bcdToDecimal((uint8)(buffer[2] & 0x3FU));
    }

    dateTime->dayOfWeek = Ds3231_bcdToDecimal((uint8)(buffer[3] & 0x07U));
    dateTime->date = Ds3231_bcdToDecimal((uint8)(buffer[4] & 0x3FU));
    dateTime->month = Ds3231_bcdToDecimal((uint8)(buffer[5] & 0x1FU));
    dateTime->year = (uint16)(2000U + Ds3231_bcdToDecimal(buffer[6]));

    return 1U;
}

uint8 Ds3231_setDateTime(const Ds3231DateTime *dateTime)
{
    if (TWI_start(DS3231_ADDRESS_WRITE) == 0U)
    {
        return 0U;
    }

    if (TWI_write(DS3231_REG_SECONDS) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    if (TWI_write(Ds3231_decimalToBcd(dateTime->second) & 0x7FU) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    if (TWI_write(Ds3231_decimalToBcd(dateTime->minute) & 0x7FU) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    if (TWI_write(Ds3231_decimalToBcd(dateTime->hour) & 0x3FU) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    if (TWI_write(Ds3231_decimalToBcd(dateTime->dayOfWeek) & 0x07U) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    if (TWI_write(Ds3231_decimalToBcd(dateTime->date) & 0x3FU) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    if (TWI_write(Ds3231_decimalToBcd(dateTime->month) & 0x1FU) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    if (TWI_write(Ds3231_decimalToBcd((uint8)(dateTime->year % 100U))) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    TWI_stop();
    return 1U;
}

uint8 Ds3231_isOscillatorStopped(uint8 *stopped)
{
    uint8 status;

    if (TWI_start(DS3231_ADDRESS_WRITE) == 0U)
    {
        return 0U;
    }

    if (TWI_write(DS3231_REG_STATUS) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    if (TWI_start(DS3231_ADDRESS_READ) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    if (TWI_readNack(&status) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    TWI_stop();
    *stopped = ((status & 0x80U) != 0U) ? 1U : 0U;
    return 1U;
}

uint8 Ds3231_clearOscillatorStop(void)
{
    uint8 status;

    if (TWI_start(DS3231_ADDRESS_WRITE) == 0U)
    {
        return 0U;
    }

    if (TWI_write(DS3231_REG_STATUS) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    if (TWI_start(DS3231_ADDRESS_READ) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    if (TWI_readNack(&status) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    TWI_stop();

    status &= (uint8)~0x80U;

    if (TWI_start(DS3231_ADDRESS_WRITE) == 0U)
    {
        return 0U;
    }

    if (TWI_write(DS3231_REG_STATUS) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    if (TWI_write(status) == 0U)
    {
        TWI_stop();
        return 0U;
    }

    TWI_stop();
    return 1U;
}