/*
 * STM32F446 RCC
 *
 * Copyright (c) 2023 Zoltan Mihaly <zoltanmihaly@gmail.com>
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

#include <stdbool.h>
#include "qemu/osdep.h"
#include "qemu/log.h"
#include "trace.h"
#include "migration/vmstate.h"
#include "qemu/timer.h"
#include "hw/qdev-clock.h"
#include <hw/robonaut/stm32f446_util.h>
#include <hw/robonaut/stm32f446_rcc.h>
#include <hw/robonaut/stm32f446_soc.h>

static void stm32f446_rcc_reset(DeviceState *dev)
{
    STM32F446RccState *s = STM32F446_RCC(dev);

	s->cr = 0x00000083; //HSI ready
	s->pllcfgr = 0x24003010;
	s->cfgr = 0x00000000;
	s->cir = 0x00000000;
	s->ahb1rstr = 0x00000000;
	s->ahb2rstr = 0x00000000;
	s->ahb3rstr = 0x00000000;
	s->apb1rstr = 0x00000000;
	s->apb2rstr = 0x00000000;
	s->ahb1enr = 0x00000000;
	s->ahb2enr = 0x00000000;
	s->ahb3enr = 0x00000000;
	s->apb1enr = 0x00000000;
	s->apb2enr = 0x00000000;
	s->ahb1lpenr = 0x606790FF;
	s->ahb2lpenr = 0x00000081;
	s->ahb3lpenr = 0x00000003;
	s->apb1lpenr = 0x3FFFC9FF;
	s->apb2lpenr = 0x00C77F33;
	s->bdcr = 0x00000000;
	s->csr = 0x0E000000;
	s->sscgr = 0x00000000;
	s->plli2scfgr = 0x24003010;
	s->pllsaicfgr = 0x04003010;
	s->dckcfgr = 0x00000000;
	s->ckgatenr = 0x00000000;
	s->dckcfgr2 = 0x00000000;

  clock_set_mul_div (s->soc->apb2clk, 1, 1);
  clock_set_mul_div (s->soc->apb2clk, 1, 1);

}

static uint64_t stm32f446_rcc_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F446RccState *s = opaque;

    switch (addr) {
    case STM32F446_RCC_CR:
        return s->cr;
    case STM32F446_RCC_PLLCFGR:
        return s->pllcfgr;
    case STM32F446_RCC_CFGR:
        return s->cfgr;
    case STM32F446_RCC_AHB1ENR:
	return s->ahb1enr;
   case STM32F446_RCC_APB1ENR:
        return s->apb1enr;
    case STM32F446_RCC_APB2ENR:
        return s->apb2enr;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, addr);
        return 0;
    }
}

static const char *deviceNme = "RCC";

#define STM32F446_RCC_CR_HSEON (1ul << 16)
#define STM32F446_RCC_CR_HSERDY (1ul << 17)
#define STM32F446_RCC_CR_PLLON (1ul << 24)
#define STM32F446_RCC_CR_PLLRDY (1ul << 25)

static void stm32f446_rcc_cr(STM32F446RccState *s, uint32_t value)
{
  uint32_t ch = value ^ s->cr;
  //HSE ON/OFF
  if (ch & STM32F446_RCC_CR_HSEON)
  {
    //instantly get ready
    bool hseon = (value & STM32F446_RCC_CR_HSEON) > 0;
    value = (value & ~STM32F446_RCC_CR_HSERDY) | (hseon ? STM32F446_RCC_CR_HSERDY : 0);
    ch &= ~(STM32F446_RCC_CR_HSEON | STM32F446_RCC_CR_HSERDY);
    qemu_log_mask(LOG_GUEST_ERROR, "RCC: CR HSEON: %d\n", hseon);
  }
  //PLL ON/OFF
  if (ch & STM32F446_RCC_CR_PLLON)
  {
    //instantly get ready
    bool pllon = (value & STM32F446_RCC_CR_PLLON) > 0;
    value = (value & ~STM32F446_RCC_CR_PLLRDY) | (pllon ? STM32F446_RCC_CR_PLLRDY : 0);
    ch &= ~(STM32F446_RCC_CR_PLLON | STM32F446_RCC_CR_PLLRDY);
    qemu_log_mask(LOG_GUEST_ERROR, "RCC: CR PLLON: %d\n", pllon);
  }

  stm32f446_util_regUnhandled(value, &ch, deviceNme, "CR");
  s->cr = value;
}

#define STM32F446_RCC_CFGR_PPRE1_BB 10
#define STM32F446_RCC_CFGR_PPRE2_BB 13
#define STM32F446_RCC_CFGR_PPRE1_MASK 7u
#define STM32F446_RCC_CFGR_PPRE2_MASK 7u
#define STM32F446_RCC_CFGR_PPRE1 (STM32F446_RCC_CFGR_PPRE1_MASK << STM32F446_RCC_CFGR_PPRE1_BB)
#define STM32F446_RCC_CFGR_PPRE2 (STM32F446_RCC_CFGR_PPRE2_MASK << STM32F446_RCC_CFGR_PPRE2_BB)
#define STM32F446_RCC_CFGR_SW 3u
#define STM32F446_RCC_CFGR_SWS 0xcu

static void stm32f446_rcc_cfgr(STM32F446RccState *s, uint32_t value)
{
  uint32_t ch = value ^ s->cfgr;

  if (ch & STM32F446_RCC_CFGR_PPRE1)
  {
    ch &= ~STM32F446_RCC_CFGR_PPRE1;
    uint32_t ppre1 = (value & STM32F446_RCC_CFGR_PPRE1) >> STM32F446_RCC_CFGR_PPRE1_BB;
    qemu_log_mask(LOG_GUEST_ERROR, "RCC: CFGR PPRE1: %d\n", ppre1);

    //0xx: AHB clock not divided
    //100: AHB clock divided by 2
    //101: AHB clock divided by 4
    //110: AHB clock divided by 8
    //111: AHB clock divided by 16
    if (ppre1 > 3) {
      stm32f446_util_clockUpdateMulDiv(s->soc->apb1timerclk, 1, 2, false);  //timer clock 2x APB1 clock
      stm32f446_util_clockUpdateMulDiv(s->soc->apb1clk, 1u << (ppre1 - 3), 1, true);
    } else {
      stm32f446_util_clockUpdateMulDiv(s->soc->apb1timerclk, 1, 1, false);
      stm32f446_util_clockUpdateMulDiv(s->soc->apb1clk, 1, 1, true);
    }

    qemu_log_mask(LOG_GUEST_ERROR, "apb1clk: %u Hz, apb1timerclk: %u Hz\n",
	stm32f446_util_clockGetChildHz(s->soc->apb1clk),
	stm32f446_util_clockGetChildHz(s->soc->apb1timerclk));
  }

  if (ch & STM32F446_RCC_CFGR_PPRE2)
  {
    ch &= ~STM32F446_RCC_CFGR_PPRE2;
    uint32_t ppre2 = (value & STM32F446_RCC_CFGR_PPRE2) >> STM32F446_RCC_CFGR_PPRE2_BB;
    qemu_log_mask(LOG_GUEST_ERROR, "RCC: CFGR PPRE2: %d\n", ppre2);

    if (ppre2 > 3) {
      stm32f446_util_clockUpdateMulDiv(s->soc->apb2timerclk, 1, 2, false);  //timer clock 2x APB1 clock
      stm32f446_util_clockUpdateMulDiv(s->soc->apb2clk, 1u << (ppre2 - 3), 1, true);
    } else {
      stm32f446_util_clockUpdateMulDiv(s->soc->apb2timerclk, 1, 1, false);
      stm32f446_util_clockUpdateMulDiv(s->soc->apb2clk, 1, 1, true);
    }
    qemu_log_mask(LOG_GUEST_ERROR, "apb2clk: %u Hz, apb2timerclk: %u Hz\n",
	stm32f446_util_clockGetChildHz(s->soc->apb2clk),
	stm32f446_util_clockGetChildHz(s->soc->apb2timerclk));

  }

  if (ch & STM32F446_RCC_CFGR_SW)
  {
    ch &= ~(STM32F446_RCC_CFGR_SW | STM32F446_RCC_CFGR_SWS);
    uint32_t sw = (value & STM32F446_RCC_CFGR_SW);
    qemu_log_mask(LOG_GUEST_ERROR, "RCC: CFGR SW: %d\n", sw);
    //instant switched
    value = (value & ~STM32F446_RCC_CFGR_SWS) | (sw << 2);
    //TODO update system clock
  }

  stm32f446_util_regUnhandled(value, &ch, deviceNme, "CFGR");
  s->cfgr = value;
}

static void stm32f446_rcc_apb1enr(STM32F446RccState *s, uint32_t value)
{
  uint32_t ch = value ^ s->apb1enr;
  stm32f446_util_regBitChange( value, &ch, deviceNme, "APB1EN", "PWREN", 28u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "APB1EN", "TIM2EN", 0u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "APB1EN", "TIM3EN", 1u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "APB1EN", "TIM4EN", 2u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "APB1EN", "TIM5EN", 3u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "APB1EN", "TIM6EN", 4u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "APB1EN", "TIM7EN", 5u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "APB1EN", "SPI2EN", 14u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "APB1EN", "I2C1EN", 21u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "APB1EN", "I2C2EN", 22u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "APB1EN", "I2C3EN", 23u);
  stm32f446_util_regUnhandled(value, &ch, deviceNme, "APB1EN");
  s->apb1enr = value;
}

static void stm32f446_rcc_apb2enr(STM32F446RccState *s, uint32_t value)
{
  uint32_t ch = value ^ s->apb2enr;
  stm32f446_util_regBitChange( value, &ch, deviceNme, "APB2EN", "TIM1EN", 0u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "APB2EN", "SPI1EN", 12u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "APB2EN", "SYSCFGEN", 14u);
  stm32f446_util_regUnhandled(value, &ch, deviceNme, "APB2EN");
  s->apb2enr = value;
}

static void stm32f446_rcc_ahb1enr(STM32F446RccState *s, uint32_t value)
{
  uint32_t ch = value ^ s->ahb1enr;
  stm32f446_util_regBitChange( value, &ch, deviceNme, "AHB1EN", "GPIOAEN", 0u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "AHB1EN", "GPIOBEN", 1u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "AHB1EN", "GPIOCEN", 2u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "AHB1EN", "GPIODEN", 3u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "AHB1EN", "GPIOEEN", 4u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "AHB1EN", "GPIOFEN", 5u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "AHB1EN", "GPIOGEN", 6u);
  stm32f446_util_regBitChange( value, &ch, deviceNme, "AHB1EN", "GPIOHEN", 7u);
  stm32f446_util_regUnhandled(value, &ch, deviceNme, "AHB1EN");
  s->ahb1enr = value;
}

static void stm32f446_rcc_pllcfgr(STM32F446RccState *s, uint32_t value)
{
  qemu_log_mask(LOG_GUEST_ERROR, "RCC: PLLCFGR: 0x%0x\n", value);
  s->pllcfgr = value;
}

static void stm32f446_rcc_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    STM32F446RccState *s = opaque;
    uint32_t value = val64;

    switch (addr) {
      case STM32F446_RCC_CR:
	stm32f446_rcc_cr(s, value);
	return;

      case STM32F446_RCC_PLLCFGR:
	stm32f446_rcc_pllcfgr(s, value);
	return;

      case STM32F446_RCC_CFGR:
	stm32f446_rcc_cfgr(s, value);
	return;

      case STM32F446_RCC_AHB1ENR:
	stm32f446_rcc_ahb1enr(s, value);
        return;

      case STM32F446_RCC_APB1ENR:
	stm32f446_rcc_apb1enr(s, value);
        return;

      case STM32F446_RCC_APB2ENR:
	stm32f446_rcc_apb2enr(s, value);
        return;

      default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%" HWADDR_PRIx ", value 0x%0*" PRIx64 "\n", __func__, addr, size << 1, val64);

    }
}

static const MemoryRegionOps stm32f446_rcc_ops = {
    .read = stm32f446_rcc_read,
    .write = stm32f446_rcc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f446_rcc_init(Object *obj)
{
    STM32F446RccState *s = STM32F446_RCC(obj);

    memory_region_init_io(&s->mmio, obj, &stm32f446_rcc_ops, s,
                          TYPE_STM32F446_RCC, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static const VMStateDescription vmstate_stm32f446_rcc = {
    .name = TYPE_STM32F446_RCC,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(cr, STM32F446RccState),
        VMSTATE_UINT32(pllcfgr, STM32F446RccState),
        VMSTATE_UINT32(cfgr, STM32F446RccState),
        VMSTATE_UINT32(cir, STM32F446RccState),
        VMSTATE_UINT32(ahb1rstr, STM32F446RccState),
        VMSTATE_UINT32(ahb2rstr, STM32F446RccState),
        VMSTATE_UINT32(ahb3rstr, STM32F446RccState),
        VMSTATE_UINT32(apb1rstr, STM32F446RccState),
        VMSTATE_UINT32(apb2rstr, STM32F446RccState),
        VMSTATE_UINT32(ahb1enr, STM32F446RccState),
        VMSTATE_UINT32(ahb2enr, STM32F446RccState),
        VMSTATE_UINT32(ahb3enr, STM32F446RccState),
        VMSTATE_UINT32(apb1enr, STM32F446RccState),
        VMSTATE_UINT32(apb2enr, STM32F446RccState),
        VMSTATE_UINT32(ahb1lpenr, STM32F446RccState),
        VMSTATE_UINT32(ahb2lpenr, STM32F446RccState),
        VMSTATE_UINT32(ahb3lpenr, STM32F446RccState),
        VMSTATE_UINT32(apb1lpenr, STM32F446RccState),
        VMSTATE_UINT32(apb2lpenr, STM32F446RccState),
        VMSTATE_UINT32(bdcr, STM32F446RccState),
        VMSTATE_UINT32(csr, STM32F446RccState),
        VMSTATE_UINT32(sscgr, STM32F446RccState),
        VMSTATE_UINT32(plli2scfgr, STM32F446RccState),
        VMSTATE_UINT32(pllsaicfgr, STM32F446RccState),
        VMSTATE_UINT32(dckcfgr, STM32F446RccState),
        VMSTATE_UINT32(ckgatenr, STM32F446RccState),
        VMSTATE_UINT32(dckcfgr2, STM32F446RccState),
        VMSTATE_END_OF_LIST()
    }
};

static void stm32f446_rcc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f446_rcc_reset;
    dc->vmsd = &vmstate_stm32f446_rcc;
}

static const TypeInfo stm32f446_rcc_info = {
    .name          = TYPE_STM32F446_RCC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F446RccState),
    .instance_init = stm32f446_rcc_init,
    .class_init    = stm32f446_rcc_class_init,
};

static void stm32f446_rcc_register_types(void)
{
    type_register_static(&stm32f446_rcc_info);
}

type_init(stm32f446_rcc_register_types)
