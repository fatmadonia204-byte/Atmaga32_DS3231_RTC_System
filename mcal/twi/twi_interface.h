#ifndef TWI_INTERFACE_H
#define TWI_INTERFACE_H

#include "../../lib/std_types.h"

void TWI_init(uint32 bitRate);
uint8 TWI_start(uint8 address);
void TWI_stop(void);
uint8 TWI_write(uint8 data);
uint8 TWI_readAck(uint8 *data);
uint8 TWI_readNack(uint8 *data);

#endif