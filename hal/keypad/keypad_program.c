#include "../keypad/keypad_interface.h"

#include "../keypad/keypad_register.h"
#include "../keypad/keypad_private.h"

#include <util/delay.h>

#include "../../mcal/dio/dio_interface.h"
#include "../../config.h"

static char Keypad_scan(void)
{
    static const char keyMap[4][4] = {
        {'7', '8', '9', 'A'},
        {'4', '5', '6', 'B'},
        {'1', '2', '3', 'C'},
        {'*', '0', '#', 'D'}
    };
    static const uint8 rowPins[4] = {
        KEYPAD_ROW_4_PIN,
        KEYPAD_ROW_3_PIN,
        KEYPAD_ROW_2_PIN,
        KEYPAD_ROW_1_PIN
    };
    static const uint8 columnPins[4] = {
        KEYPAD_COL_1_PIN,
        KEYPAD_COL_2_PIN,
        KEYPAD_COL_3_PIN,
        KEYPAD_COL_4_PIN
    };

    uint8 row;
    uint8 column;

    for (row = 0U; row < 4U; row++)
    {
        DIO_setPortValue(DIO_PORTB_ID, 0xFFU);
        DIO_setPinValue(DIO_PORTB_ID, rowPins[row], 0U);
        _delay_us(2);

        for (column = 0U; column < 4U; column++)
        {
            if (BIT_IS_CLEAR(KEYPAD_PIN, columnPins[column]))
            {
                return keyMap[3U - row][column];
            }
        }
    }

    return 0;
}

void Keypad_init(void)
{
    DIO_setPortDirection(DIO_PORTB_ID, 0x0FU);
    DIO_setPortValue(DIO_PORTB_ID, 0xFFU);
}

char Keypad_getKey(void)
{
    static char lastSample = 0;
    static char lockedKey = 0;
    static uint8 stableCount = 0U;
    char current = Keypad_scan();

    if (current == lastSample)
    {
        if (stableCount < 3U)
        {
            stableCount++;
        }
    }
    else
    {
        lastSample = current;
        stableCount = 0U;
    }

    if (lockedKey != 0)
    {
        if (current == 0)
        {
            lockedKey = 0;
        }

        return 0;
    }

    if ((current != 0) && (stableCount >= 2U))
    {
        lockedKey = current;
        return current;
    }

    return 0;
}