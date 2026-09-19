#include "../buzzer/buzzer_interface.h"

#include "../buzzer/buzzer_register.h"
#include "../buzzer/buzzer_private.h"

#include "../../config.h"

static volatile uint8 g_buzzerActive = 0U;

void Buzzer_init(void)
{
    SETBIT(BUZZER_DDR, BUZZER_PIN);
    CLRBIT(BUZZER_PORT, BUZZER_PIN);
}

void Buzzer_start(void)
{
    g_buzzerActive = 1U;
    SETBIT(BUZZER_DDR, BUZZER_PIN);
    SETBIT(BUZZER_PORT, BUZZER_PIN);
}

void Buzzer_stop(void)
{
    CLRBIT(BUZZER_PORT, BUZZER_PIN);
    g_buzzerActive = 0U;
}

uint8 Buzzer_isOn(void)
{
    return g_buzzerActive;
}