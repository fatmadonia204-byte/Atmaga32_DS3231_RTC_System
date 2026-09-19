#include "twi_interface.h"

#include "twi_register.h"
#include "twi_private.h"

#include "../../config.h"

static uint8 TWI_waitForInterrupt(void)
{
    uint16 timeout = 60000U;

    while ((BIT_IS_CLEAR(TWCR, TWINT) != 0U) && (timeout > 0U))
    {
        timeout--;
    }

    return (timeout > 0U) ? 1U : 0U;
}

void TWI_init(uint32 bitRate)
{
    CLRBIT(DDRC, PC0);
    CLRBIT(DDRC, PC1);
    SETBIT(PORTC, PC0);
    SETBIT(PORTC, PC1);
    CLRBIT(TWSR, TWPS1);
    CLRBIT(TWSR, TWPS0);
    WRITE_REG(TWBR, (uint8)(((F_CPU / bitRate) - 16UL) / 2UL));
    SETBIT(TWCR, TWEN);
}

uint8 TWI_start(uint8 address)
{
    WRITE_REG(TWCR, (1U << TWINT) | (1U << TWSTA) | (1U << TWEN));

    if (TWI_waitForInterrupt() == 0U)
    {
        TWI_stop();
        return 0U;
    }

    if ((TWSR & TWI_STATUS_MASK) != TWI_STATUS_START && (TWSR & TWI_STATUS_MASK) != TWI_STATUS_REPEATED_START)
    {
        TWI_stop();
        return 0U;
    }

    WRITE_REG(TWDR, address);
    WRITE_REG(TWCR, (1U << TWINT) | (1U << TWEN));

    if (TWI_waitForInterrupt() == 0U)
    {
        TWI_stop();
        return 0U;
    }

    if ((address & 1U) == 0U)
    {
        if ((TWSR & TWI_STATUS_MASK) != TWI_STATUS_SLA_W_ACK)
        {
            TWI_stop();
            return 0U;
        }
    }
    else if ((TWSR & TWI_STATUS_MASK) != TWI_STATUS_SLA_R_ACK)
    {
        TWI_stop();
        return 0U;
    }

    return 1U;
}

void TWI_stop(void)
{
    WRITE_REG(TWCR, (1U << TWINT) | (1U << TWSTO) | (1U << TWEN));
}

uint8 TWI_write(uint8 data)
{
    WRITE_REG(TWDR, data);
    WRITE_REG(TWCR, (1U << TWINT) | (1U << TWEN));

    if (TWI_waitForInterrupt() == 0U)
    {
        return 0U;
    }

    return ((TWSR & TWI_STATUS_MASK) == TWI_STATUS_DATA_W_ACK) ? 1U : 0U;
}

uint8 TWI_readAck(uint8 *data)
{
    WRITE_REG(TWCR, (1U << TWINT) | (1U << TWEN) | (1U << TWEA));

    if (TWI_waitForInterrupt() == 0U)
    {
        return 0U;
    }

    *data = TWDR;
    return ((TWSR & TWI_STATUS_MASK) == TWI_STATUS_DATA_R_ACK) ? 1U : 0U;
}

uint8 TWI_readNack(uint8 *data)
{
    WRITE_REG(TWCR, (1U << TWINT) | (1U << TWEN));

    if (TWI_waitForInterrupt() == 0U)
    {
        return 0U;
    }

    *data = TWDR;
    return ((TWSR & TWI_STATUS_MASK) == TWI_STATUS_DATA_R_NACK) ? 1U : 0U;
}