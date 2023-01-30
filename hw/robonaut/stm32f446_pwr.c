/*
 * STM32F446 PWR
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
#include <hw/robonaut/stm32f446_pwr.h>
#include <hw/robonaut/stm32f446_soc.h>

static void stm32f446_pwr_reset(DeviceState *dev)
{
  STM32F446PwrState *s = STM32F446_PWR(dev);

  s->cr = 0x0000C000;
  s->csr = 0x00000000;

}

static uint64_t stm32f446_pwr_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F446PwrState *s = opaque;

/*    qemu_log("PWR: read "
                  "(size %d, addr 0x%0*" HWADDR_PRIx ") @ %" PRId64 "\n",
                  size, 4, addr, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL));
*/
    //qemu_log("timeinfo - qemu_clock_get_ns: %" PRId64 ", ", qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL), icount_get());

    switch (addr) {
    case STM32F446_PWR_CR:
        return s->cr;
    case STM32F446_PWR_CSR:
        return s->csr;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, addr);
        return 0;
    }
}

#define STM32F446_PWR_CR_VOIS (3ul << 14)
#define STM32F446_PWR_CR_ODEN (1ul << 16)
#define STM32F446_PWR_CR_ODSWEN (1ul << 17)
#define STM32F446_PWR_CSR_ODRDY (1ul << 16)
#define STM32F446_PWR_CSR_ODSWRDY (1ul << 17)

static void stm32f446_pwr_cr(STM32F446PwrState *s, uint32_t value)
{
  uint32_t ch = value ^ s->cr;
  //HSE ON/OFF
  if (ch & STM32F446_PWR_CR_VOIS)
  {
    uint32_t newScale = (value & STM32F446_PWR_CR_VOIS) >> 14;
    ch &= ~STM32F446_PWR_CR_VOIS;
    qemu_log_mask(LOG_GUEST_ERROR, "PWR: CR voltage scale: %d\n", newScale);
  }
  //OVERDRIVE ON/OFF
  if (ch & (STM32F446_PWR_CR_ODEN | STM32F446_PWR_CR_ODSWEN))
  {
    ch &= ~(STM32F446_PWR_CR_ODEN | STM32F446_PWR_CR_ODSWEN);
    bool oden = (value & STM32F446_PWR_CR_ODEN) > 0;
    bool odswen = (value & STM32F446_PWR_CR_ODSWEN) > 0;

    qemu_log_mask(LOG_GUEST_ERROR, "PWR: CR ODEN: %d, ODSWEN: %d\n", oden, odswen);

    //instantly get ready
    s->csr = (s->csr & ~(STM32F446_PWR_CSR_ODRDY | STM32F446_PWR_CSR_ODSWRDY))
	| (
	    oden ? (
		odswen ? STM32F446_PWR_CSR_ODRDY | STM32F446_PWR_CSR_ODSWRDY : STM32F446_PWR_CSR_ODRDY
	    ) : 0
	);

  }

  if (ch > 0)
  {
    qemu_log_mask(LOG_GUEST_ERROR, "PWR: CR UNHANDLED change: 0x%0x, value: 0x%0x\n", ch, value);

  }
  s->cr = value;
}

static void stm32f446_pwr_csr(STM32F446PwrState *s, uint32_t value)
{
  uint32_t ch = value ^ s->csr;
  if (ch > 0)
  {
    qemu_log_mask(LOG_GUEST_ERROR, "PWR: CSR UNHANDLED change: 0x%0x, value: 0x%0x\n", ch, value);

  }
  s->csr = value;
}

static void stm32f446_pwr_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    STM32F446PwrState *s = opaque;
    uint32_t value = val64;

    switch (addr) {
      case STM32F446_PWR_CR:
	stm32f446_pwr_cr(s, value);
	return;

      case STM32F446_PWR_CSR:
	stm32f446_pwr_csr(s, value);
	return;

      default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, addr);
    }
}

static const MemoryRegionOps stm32f446_pwr_ops = {
    .read = stm32f446_pwr_read,
    .write = stm32f446_pwr_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f446_pwr_init(Object *obj)
{
    STM32F446PwrState *s = STM32F446_PWR(obj);

    memory_region_init_io(&s->mmio, obj, &stm32f446_pwr_ops, s, TYPE_STM32F446_PWR, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static const VMStateDescription vmstate_stm32f446_pwr = {
    .name = TYPE_STM32F446_PWR,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(cr, STM32F446PwrState),
        VMSTATE_UINT32(csr, STM32F446PwrState),
        VMSTATE_END_OF_LIST()
    }
};

static void stm32f446_pwr_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f446_pwr_reset;
    dc->vmsd = &vmstate_stm32f446_pwr;
}

static const TypeInfo stm32f446_pwr_info = {
    .name          = TYPE_STM32F446_PWR,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F446PwrState),
    .instance_init = stm32f446_pwr_init,
    .class_init    = stm32f446_pwr_class_init,
};

static void stm32f446_pwr_register_types(void)
{
    type_register_static(&stm32f446_pwr_info);
}

type_init(stm32f446_pwr_register_types)
