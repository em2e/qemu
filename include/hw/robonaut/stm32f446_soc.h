/*
 * STM32F405 SoC
 *
 * Copyright (c) 2014 Alistair Francis <alistair@alistair23.me>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef HW_ARM_STM32F446_SOC_H
#define HW_ARM_STM32F446_SOC_H

#include "hw/misc/stm32f4xx_syscfg.h"
#include "hw/timer/stm32f2xx_timer.h"
#include "hw/char/stm32f2xx_usart.h"
#include "hw/adc/stm32f2xx_adc.h"
#include "hw/misc/stm32f4xx_exti.h"
#include "hw/or-irq.h"
#include "hw/ssi/stm32f2xx_spi.h"
#include "hw/arm/armv7m.h"
#include "qom/object.h"
#include "stm32f4xx_flash.h"
#include "stm32f4xx_rcc.h"

#define TYPE_STM32F446RE_SOC "stm32f446re-soc"
#define TYPE_STM32F446_SOC "stm32f446-soc"
OBJECT_DECLARE_SIMPLE_TYPE(STM32F446State, STM32F446_SOC)

#define STM_NUM_USARTS 6 /* 2 UART */
#define STM_NUM_TIMERS 14 /* 10 GP, 2 advanced, 2 basic */
#define STM_NUM_ADCS 3 /* 16 channel */
#define STM_NUM_SPIS 4

#define FLASH_BASE_ADDRESS 0x08000000
#define FLASH_SIZE_E (512 * 1024)
#define FLASH_SIZE_C (256 * 1024)
#define SRAM_BASE_ADDRESS 0x20000000
#define SRAM_SIZE (128 * 1024)

#define STM32F446MC 1
#define STM32F446ME 2
#define STM32F446RC 3
#define STM32F446RE 4
#define STM32F446VC 5
#define STM32F446VE 6
#define STM32F446ZC 7
#define STM32F446ZE 8

struct STM32F446State {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    char *cpu_type;

    ARMv7MState armv7m;

    int32_t variant;

    STM32F4xxSyscfgState syscfg;
    STM32F4xxExtiState exti;
    STM32F2XXUsartState usart[STM_NUM_USARTS];
    STM32F2XXTimerState timer[STM_NUM_TIMERS];
    qemu_or_irq adc_irqs;
    STM32F2XXADCState adc[STM_NUM_ADCS];
    STM32F2XXSPIState spi[STM_NUM_SPIS];
    STM32F4XXFlashState flashInterface;
    STM32F4XXRccState rcc;

    MemoryRegion sram;
    MemoryRegion flash;
    MemoryRegion flash_alias;

    Clock *sysclk;
    Clock *refclk;
};

#endif
