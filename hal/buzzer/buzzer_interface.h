#ifndef BUZZER_INTERFACE_H
#define BUZZER_INTERFACE_H

#include "../../lib/std_types.h"

void Buzzer_init(void);
void Buzzer_start(void);
void Buzzer_stop(void);
uint8 Buzzer_isOn(void);

#endif