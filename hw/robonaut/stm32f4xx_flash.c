/*
 * STM32F4xx FLASH
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
#include "hw/robonaut/stm32f4xx_flash.h"

static void stm32f4xx_flash_reset(DeviceState *dev)
{
    STM32F4XXFlashState *s = STM32F4XX_FLASH(dev);

    s->acr = 0x00000000;
    s->keyr = 0x00000000;
    s->optkeyr = 0x00000000;
    s->sr = 0x00000000;
    s->cr = 0x80000000;
    s->optcr = 0x00ffaaed;
}

static uint64_t stm32f4xx_flash_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
    STM32F4XXFlashState *s = opaque;

    qemu_log_mask(LOG_GUEST_ERROR, "FLASH: read "
                  "(size %d, addr 0x%0*" HWADDR_PRIx ")\n",
                  size, 4, addr);

    switch (addr) {
    case SMT32F4XX_FLASH_ACR:
        return s->acr;
    case SMT32F4XX_FLASH_KEYR:
        return 0;
    case SMT32F4XX_FLASH_OPTKEYR:
        return 0;
    case SMT32F4XX_FLASH_SR:
        return s->sr;
    case SMT32F4XX_FLASH_CR:
        return s->cr;
    case SMT32F4XX_FLASH_OPTCR:
        return s->optcr;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, addr);
        return 0;
    }
}

static void stm32f4xx_flash_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
    STM32F4XXFlashState *s = opaque;
    uint32_t value = val64;

    qemu_log_mask(LOG_GUEST_ERROR, "FLASH: write "
                  "(size %d, offset 0x%0*" HWADDR_PRIx
                  ", value 0x%0*" PRIx64 ")\n",
                  size, 4, addr, size << 1, val64);

    switch (addr) {
    case SMT32F4XX_FLASH_ACR:
        s->acr = value;
        return;
    case SMT32F4XX_FLASH_KEYR:
    	if (s->keyr == SMT32F4XX_FLASH_KEY1 && value == SMT32F4XX_FLASH_KEY2) {
    		s->cr &= 0x7fffffff;
    	    qemu_log_mask(LOG_GUEST_ERROR, "FLASH: FLASH_CR is unlocked");
    	}
    	s->keyr = value;
        return;
    case SMT32F4XX_FLASH_OPTKEYR:
    	if (s->optkeyr == SMT32F4XX_FLASH_OPTKEY1 && value == SMT32F4XX_FLASH_OPTKEY2) {
    		s->optcr &= 0x00000001;
    	    qemu_log_mask(LOG_GUEST_ERROR, "FLASH: FLASH_OPTCR is unlocked");
    	}
        s->optkeyr = value;
        return;
    case SMT32F4XX_FLASH_SR:
        s->sr = 0;
        return;
    case SMT32F4XX_FLASH_CR:
        s->cr  = value |= 0x80000000;
        return;
    case SMT32F4XX_FLASH_OPTCR:
        s->optcr = value |= 0x00000001;
        return;
/*
    case SYSCFG_MEMRMP:
        qemu_log_mask(LOG_UNIMP,
                      "%s: Changing the memory mapping isn't supported " \
                      "in QEMU\n", __func__);
        return;
    case SYSCFG_PMC:
        qemu_log_mask(LOG_UNIMP,
                      "%s: Changing the memory mapping isn't supported " \
                      "in QEMU\n", __func__);
        return;
    case SYSCFG_EXTICR1...SYSCFG_EXTICR4:
        s->syscfg_exticr[addr / 4 - SYSCFG_EXTICR1 / 4] = (value & 0xFFFF);
        return;
    case SYSCFG_CMPCR:
        s->syscfg_cmpcr = value;
        return;
 */
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", __func__, addr);
    }
}

static const MemoryRegionOps stm32f4xx_flash_ops = {
    .read = stm32f4xx_flash_read,
    .write = stm32f4xx_flash_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f4xx_flash_init(Object *obj)
{
    STM32F4XXFlashState *s = STM32F4XX_FLASH(obj);

    //sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    memory_region_init_io(&s->mmio, obj, &stm32f4xx_flash_ops, s,
                          TYPE_STM32F4XX_FLASH, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);

    //qdev_init_gpio_in(DEVICE(obj), stm32f4xx_syscfg_set_irq, 16 * 9);
    //qdev_init_gpio_out(DEVICE(obj), s->gpio_out, 16);
}

static const VMStateDescription vmstate_stm32f4xx_flash = {
    .name = TYPE_STM32F4XX_FLASH,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(acr, STM32F4XXFlashState),
		VMSTATE_UINT32(keyr, STM32F4XXFlashState),
		VMSTATE_UINT32(optkeyr, STM32F4XXFlashState),
		VMSTATE_UINT32(sr, STM32F4XXFlashState),
		VMSTATE_UINT32(cr, STM32F4XXFlashState),
		VMSTATE_UINT32(optcr, STM32F4XXFlashState),
        VMSTATE_END_OF_LIST()
    }
};

static void stm32f4xx_flash_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f4xx_flash_reset;
    dc->vmsd = &vmstate_stm32f4xx_flash;
}

static const TypeInfo stm32f4xx_flash_info = {
    .name          = TYPE_STM32F4XX_FLASH,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F4XXFlashState),
    .instance_init = stm32f4xx_flash_init,
    .class_init    = stm32f4xx_flash_class_init,
};

static void stm32f4xx_flash_register_types(void)
{
    type_register_static(&stm32f4xx_flash_info);
}

type_init(stm32f4xx_flash_register_types)
