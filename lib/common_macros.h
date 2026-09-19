#ifndef COMMON_MACROS_H
#define COMMON_MACROS_H

#include <stdint.h>

#define SETBIT(reg, bit)        ((reg) |= (uint8_t)(1U << (bit)))
#define CLRBIT(reg, bit)        ((reg) &= (uint8_t)~(uint8_t)(1U << (bit)))
#define TGLBIT(reg, bit)        ((reg) ^= (uint8_t)(1U << (bit)))
#define GETBIT(reg, bit)        (((reg) >> (bit)) & 1U)
#define BIT_IS_SET(reg, bit)    (GETBIT((reg), (bit)) != 0U)
#define BIT_IS_CLEAR(reg, bit)  (GETBIT((reg), (bit)) == 0U)

#define WRITE_REG(reg, value)       ((reg) = (uint8_t)(value))
#define SET_REG_BITS(reg, mask)     ((reg) |= (uint8_t)(mask))
#define CLEAR_REG_BITS(reg, mask)   ((reg) &= (uint8_t)~(uint8_t)(mask))

#define SET_PIN_DIR(ddr, pin)       SETBIT((ddr), (pin))
#define CLEAR_PIN_DIR(ddr, pin)     CLRBIT((ddr), (pin))
#define SET_PORT_DIR(ddr, mask)     SET_REG_BITS((ddr), (mask))
#define CLEAR_PORT_DIR(ddr, mask)   CLEAR_REG_BITS((ddr), (mask))

#define SET_PIN_LVL(port, pin)      SETBIT((port), (pin))
#define CLEAR_PIN_LVL(port, pin)    CLRBIT((port), (pin))
#define TOGGLE_PIN_LVL(port, pin)   TGLBIT((port), (pin))
#define SET_PORT_LVL(port, mask)    SET_REG_BITS((port), (mask))
#define CLEAR_PORT_LVL(port, mask)  CLEAR_REG_BITS((port), (mask))

#define MAX_VALUE(a, b)         (((a) > (b)) ? (a) : (b))
#define MIN_VALUE(a, b)         (((a) < (b)) ? (a) : (b))

#endif