#ifndef KEYPAD_INTERFACE_H
#define KEYPAD_INTERFACE_H

#include "../../lib/std_types.h"

void Keypad_init(void);
char Keypad_getKey(void);

#endif