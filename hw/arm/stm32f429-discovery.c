/*
 * ST STM32F429I-DISCO (STM32F429I-DISC1) board
 *
 * Based on the STM32F405 SoC model with the STM32F429 memory sizes,
 * plus the board's 8 MiB IS42S16400J SDRAM on FMC bank 2.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/core/boards.h"
#include "hw/core/qdev-properties.h"
#include "hw/core/qdev-clock.h"
#include "qemu/error-report.h"
#include "hw/arm/stm32f405_soc.h"
#include "hw/arm/boot.h"
#include "hw/misc/unimp.h"
#include "system/address-spaces.h"
#include "system/system.h"

#define SYSCLK_FREQ         180000000
/* APB1 timer clock with the 168 MHz PLL setup used by afboot-stm32 */
#define TIMER_FREQ          84000000

#define STM32F429_FLASH_SIZE (2 * 1024 * 1024)
#define STM32F429_SRAM_SIZE  (192 * 1024)   /* SRAM1 + SRAM2 + SRAM3 */

/*
 * The SDRAM sits on FMC SDRAM bank 2 at 0xD0000000.  Linux, U-Boot and
 * afboot-stm32 set SYSCFG_MEMRMP.SWP_FMC and use it at 0x90000000, so
 * the RAM is mapped there and aliased at its reset address.
 */
#define FMC_SDRAM_BASE       0x90000000
#define FMC_SDRAM_ALIAS_BASE 0xD0000000
#define FMC_SDRAM_SIZE       (8 * 1024 * 1024)

static void stm32f429discovery_init(MachineState *machine)
{
    DeviceState *dev;
    Clock *sysclk;
    MemoryRegion *sdram = g_new(MemoryRegion, 1);
    MemoryRegion *sdram_alias = g_new(MemoryRegion, 1);

    sysclk = clock_new(OBJECT(machine), "SYSCLK");
    clock_set_hz(sysclk, SYSCLK_FREQ);

    dev = qdev_new(TYPE_STM32F405_SOC);
    object_property_add_child(OBJECT(machine), "soc", OBJECT(dev));
    qdev_prop_set_uint32(dev, "flash-size", STM32F429_FLASH_SIZE);
    qdev_prop_set_uint32(dev, "sram-size", STM32F429_SRAM_SIZE);
    qdev_prop_set_uint64(dev, "timer-freq", TIMER_FREQ);
    qdev_connect_clock_in(dev, "sysclk", sysclk);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);

    memory_region_init_ram(sdram, NULL, "stm32f429.sdram",
                           FMC_SDRAM_SIZE, &error_fatal);
    memory_region_add_subregion(get_system_memory(), FMC_SDRAM_BASE, sdram);
    memory_region_init_alias(sdram_alias, NULL, "stm32f429.sdram.alias",
                             sdram, 0, FMC_SDRAM_SIZE);
    memory_region_add_subregion(get_system_memory(), FMC_SDRAM_ALIAS_BASE,
                                sdram_alias);

    create_unimplemented_device("GPIOJ", 0x40022400, 0x400);
    create_unimplemented_device("GPIOK", 0x40022800, 0x400);

    armv7m_load_kernel(STM32F405_SOC(dev)->armv7m.cpu,
                       machine->kernel_filename,
                       0, STM32F429_FLASH_SIZE);
}

static void stm32f429discovery_machine_init(MachineClass *mc)
{
    static const char * const valid_cpu_types[] = {
        ARM_CPU_TYPE_NAME("cortex-m4"),
        NULL
    };

    mc->desc = "ST STM32F429I-DISCO (Cortex-M4)";
    mc->init = stm32f429discovery_init;
    mc->valid_cpu_types = valid_cpu_types;
}

DEFINE_MACHINE("stm32f429discovery", stm32f429discovery_machine_init)
