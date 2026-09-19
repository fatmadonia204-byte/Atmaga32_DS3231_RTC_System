#include "timer0_interface.h"

#include <avr/interrupt.h>

#include "timer0_register.h"
#include "timer0_private.h"

#include "../../config.h"

static volatile uint32 g_millis = 0U;

void Timer0_init1ms(void)
{
    WRITE_REG(TIFR, (uint8)(1U << OCF0));
    WRITE_REG(TIMSK, 0U);
    WRITE_REG(TCCR0, 0U);
    SETBIT(TCCR0, WGM01);
    SETBIT(TCCR0, CS01);
    SETBIT(TCCR0, CS00);
    WRITE_REG(OCR0, (uint8)(((F_CPU / 64UL) / 1000UL) - 1UL));
    SETBIT(TIMSK, OCIE0);
}

uint32 Timer0_getMillis(void)
{
    uint32 millis;

    cli();
    millis = g_millis;
    sei();

    return millis;
}

ISR(BADISR_vect)
{
    while (1)
    {
        ;
    }
}

ISR(TIMER0_COMP_vect)
{
    g_millis++;
}