/*
 * STM32F4xx true random number generator (RNG)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_MISC_STM32F4XX_RNG_H
#define HW_MISC_STM32F4XX_RNG_H

#include "hw/core/sysbus.h"
#include "qom/object.h"

#define TYPE_STM32F4XX_RNG "stm32f4xx-rng"
OBJECT_DECLARE_SIMPLE_TYPE(STM32F4xxRngState, STM32F4XX_RNG)

#define STM32F4XX_RNG_CR  0x00
#define STM32F4XX_RNG_SR  0x04
#define STM32F4XX_RNG_DR  0x08

struct STM32F4xxRngState {
    SysBusDevice parent_obj;

    MemoryRegion mmio;
    qemu_irq irq;

    uint32_t cr;
};

#endif
