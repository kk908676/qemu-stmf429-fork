/*
 * STM32F4xx true random number generator (RNG)
 *
 * Minimal model (RM0090 section 24): once RNGEN is set a fresh random
 * word is always ready, so DRDY reads as 1 and SECS/CECS never fire.
 * Data comes from QEMU's guest random source.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/guest-random.h"
#include "hw/core/irq.h"
#include "hw/misc/stm32f4xx_rng.h"
#include "migration/vmstate.h"

#define RNG_CR_RNGEN  (1 << 2)
#define RNG_CR_IE     (1 << 3)
#define RNG_SR_DRDY   (1 << 0)

static void stm32f4xx_rng_update_irq(STM32F4xxRngState *s)
{
    qemu_set_irq(s->irq, (s->cr & RNG_CR_RNGEN) && (s->cr & RNG_CR_IE));
}

static uint64_t stm32f4xx_rng_read(void *opaque, hwaddr addr, unsigned size)
{
    STM32F4xxRngState *s = opaque;
    uint32_t value = 0;

    switch (addr) {
    case STM32F4XX_RNG_CR:
        return s->cr;
    case STM32F4XX_RNG_SR:
        return (s->cr & RNG_CR_RNGEN) ? RNG_SR_DRDY : 0;
    case STM32F4XX_RNG_DR:
        if (s->cr & RNG_CR_RNGEN) {
            qemu_guest_getrandom_nofail(&value, sizeof(value));
        }
        return value;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: bad offset 0x%" HWADDR_PRIx "\n",
                      __func__, addr);
        return 0;
    }
}

static void stm32f4xx_rng_write(void *opaque, hwaddr addr, uint64_t value,
                                unsigned size)
{
    STM32F4xxRngState *s = opaque;

    switch (addr) {
    case STM32F4XX_RNG_CR:
        s->cr = value & (RNG_CR_RNGEN | RNG_CR_IE);
        stm32f4xx_rng_update_irq(s);
        return;
    case STM32F4XX_RNG_SR:
        /* SEIS/CEIS are write-0-to-clear and never set in this model */
        return;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: bad offset 0x%" HWADDR_PRIx "\n",
                      __func__, addr);
    }
}

static const MemoryRegionOps stm32f4xx_rng_ops = {
    .read = stm32f4xx_rng_read,
    .write = stm32f4xx_rng_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static void stm32f4xx_rng_reset(DeviceState *dev)
{
    STM32F4xxRngState *s = STM32F4XX_RNG(dev);

    s->cr = 0;
    stm32f4xx_rng_update_irq(s);
}

static void stm32f4xx_rng_init(Object *obj)
{
    STM32F4xxRngState *s = STM32F4XX_RNG(obj);

    memory_region_init_io(&s->mmio, obj, &stm32f4xx_rng_ops, s,
                          TYPE_STM32F4XX_RNG, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);
}

static const VMStateDescription vmstate_stm32f4xx_rng = {
    .name = TYPE_STM32F4XX_RNG,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        VMSTATE_UINT32(cr, STM32F4xxRngState),
        VMSTATE_END_OF_LIST()
    }
};

static void stm32f4xx_rng_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd = &vmstate_stm32f4xx_rng;
    device_class_set_legacy_reset(dc, stm32f4xx_rng_reset);
}

static const TypeInfo stm32f4xx_rng_info = {
    .name          = TYPE_STM32F4XX_RNG,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F4xxRngState),
    .instance_init = stm32f4xx_rng_init,
    .class_init    = stm32f4xx_rng_class_init,
};

static void stm32f4xx_rng_register_types(void)
{
    type_register_static(&stm32f4xx_rng_info);
}

type_init(stm32f4xx_rng_register_types)
