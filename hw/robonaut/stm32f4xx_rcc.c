/*
 * STM32F4xx RCC
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

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "trace.h"
#include "migration/vmstate.h"
#include "hw/robonaut/stm32f4xx_rcc.h"
#include "qemu/timer.h"

static void stm32f4xx_rcc_reset(DeviceState *dev)
{
    STM32F4XXRccState *s = STM32F4XX_RCC(dev);

	s->cr = 0x00000083;
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
}

static uint64_t stm32f4xx_rcc_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F4XXRccState *s = opaque;

    qemu_log("RCC: read "
                  "(size %d, addr 0x%0*" HWADDR_PRIx ") @ %" PRId64 "\n",
                  size, 4, addr, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL));

    //qemu_log("timeinfo - qemu_clock_get_ns: %" PRId64 ", ", qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL), icount_get());

    switch (addr) {
    case STM32F4XX_RCC_CFGR:
        return s->cfgr;
    case STM32F4XX_RCC_APB1ENR:
        return s->apb1enr;
    case STM32F4XX_RCC_APB2ENR:
        return s->apb2enr;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, addr);
        return 0;
    }
}

static void stm32f4xx_rcc_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    STM32F4XXRccState *s = opaque;
    uint32_t value = val64;

    qemu_log_mask(LOG_GUEST_ERROR, "RCC: write "
                  "(size %d, offset 0x%0*" HWADDR_PRIx
                  ", value 0x%0*" PRIx64 ")\n",
                  size, 4, addr, size << 1, val64);

    switch (addr) {
    case STM32F4XX_RCC_APB1ENR:
        s->apb1enr = value;
        return;

    case STM32F4XX_RCC_APB2ENR:
        s->apb2enr = value;
        return;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, addr);
    }
}

static const MemoryRegionOps stm32f4xx_rcc_ops = {
    .read = stm32f4xx_rcc_read,
    .write = stm32f4xx_rcc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f4xx_rcc_init(Object *obj)
{
    STM32F4XXRccState *s = STM32F4XX_RCC(obj);

    memory_region_init_io(&s->mmio, obj, &stm32f4xx_rcc_ops, s,
                          TYPE_STM32F4XX_RCC, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static const VMStateDescription vmstate_stm32f4xx_rcc = {
    .name = TYPE_STM32F4XX_RCC,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(cr, STM32F4XXRccState),
        VMSTATE_UINT32(pllcfgr, STM32F4XXRccState),
        VMSTATE_UINT32(cfgr, STM32F4XXRccState),
        VMSTATE_UINT32(cir, STM32F4XXRccState),
        VMSTATE_UINT32(ahb1rstr, STM32F4XXRccState),
        VMSTATE_UINT32(ahb2rstr, STM32F4XXRccState),
        VMSTATE_UINT32(ahb3rstr, STM32F4XXRccState),
        VMSTATE_UINT32(apb1rstr, STM32F4XXRccState),
        VMSTATE_UINT32(apb2rstr, STM32F4XXRccState),
        VMSTATE_UINT32(ahb1enr, STM32F4XXRccState),
        VMSTATE_UINT32(ahb2enr, STM32F4XXRccState),
        VMSTATE_UINT32(ahb3enr, STM32F4XXRccState),
        VMSTATE_UINT32(apb1enr, STM32F4XXRccState),
        VMSTATE_UINT32(apb2enr, STM32F4XXRccState),
        VMSTATE_UINT32(ahb1lpenr, STM32F4XXRccState),
        VMSTATE_UINT32(ahb2lpenr, STM32F4XXRccState),
        VMSTATE_UINT32(ahb3lpenr, STM32F4XXRccState),
        VMSTATE_UINT32(apb1lpenr, STM32F4XXRccState),
        VMSTATE_UINT32(apb2lpenr, STM32F4XXRccState),
        VMSTATE_UINT32(bdcr, STM32F4XXRccState),
        VMSTATE_UINT32(csr, STM32F4XXRccState),
        VMSTATE_UINT32(sscgr, STM32F4XXRccState),
        VMSTATE_UINT32(plli2scfgr, STM32F4XXRccState),
        VMSTATE_UINT32(pllsaicfgr, STM32F4XXRccState),
        VMSTATE_UINT32(dckcfgr, STM32F4XXRccState),
        VMSTATE_UINT32(ckgatenr, STM32F4XXRccState),
        VMSTATE_UINT32(dckcfgr2, STM32F4XXRccState),
        VMSTATE_END_OF_LIST()
    }
};

static void stm32f4xx_rcc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f4xx_rcc_reset;
    dc->vmsd = &vmstate_stm32f4xx_rcc;
}

static const TypeInfo stm32f4xx_rcc_info = {
    .name          = TYPE_STM32F4XX_RCC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F4XXRccState),
    .instance_init = stm32f4xx_rcc_init,
    .class_init    = stm32f4xx_rcc_class_init,
};

static void stm32f4xx_rcc_register_types(void)
{
    type_register_static(&stm32f4xx_rcc_info);
}

type_init(stm32f4xx_rcc_register_types)
