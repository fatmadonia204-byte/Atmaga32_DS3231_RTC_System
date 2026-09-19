#include <avr/interrupt.h>

#include "app/rtc_app.h"

int main(void)
{
    RTC_APP_init();
    sei();

    while (1)
    {
        RTC_APP_task();
    }

    return 0;
}