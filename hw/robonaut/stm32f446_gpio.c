/*
 * STM32F446 GPIO
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
#include "hw/qdev-properties.h"
#include <hw/robonaut/stm32f446_util.h>
#include <hw/robonaut/stm32f446_gpio.h>
//#include <hw/robonaut/stm32f446_soc.h>

static void bsrrHandler(void *device, uint32_t *ch, uint32_t *value) {
  STM32F446GpioState *s = device;

  uint16_t bs = *value & 0xffff;
	uint16_t br = (*value >> 16) & ~bs;
	*ch = 0;
	*value = 0;
	s->odr = (s->odr | bs) & ~br;
}

static const RegisterInfo regInfo[] = {
	{.name = "MODER",    .hwaddr = 0x00, .initValue = 0x0, .regOffs = offsetof(STM32F446GpioState, moder) },
	{.name = "OTYPER",   .hwaddr = 0x04, .initValue = 0x0, .regOffs = offsetof(STM32F446GpioState, otyper), .log = true},
	{.name = "OSPEEDER", .hwaddr = 0x08, .initValue = 0x0, .regOffs = offsetof(STM32F446GpioState, ospeeder), .log = true},
	{.name = "PUPDR",    .hwaddr = 0x0c, .initValue = 0x0, .regOffs = offsetof(STM32F446GpioState, puprd), .log = true},
	{.name = "IDR",      .hwaddr = 0x10, .initValue = 0x0, .regOffs = offsetof(STM32F446GpioState, idr), .log = false, .readOnly = true},
	{.name = "ODR",      .hwaddr = 0x14, .initValue = 0x0, .regOffs = offsetof(STM32F446GpioState, odr), .log = false},
	{.name = "BSRR",     .hwaddr = 0x18, .initValue = 0x0, .regOffs = offsetof(STM32F446GpioState, bsrr), .log = false, .wrHandler = bsrrHandler},
	{.name = "LCKR",     .hwaddr = 0x1c, .initValue = 0x0, .regOffs = offsetof(STM32F446GpioState, lckr), .log = true},
	{.name = "AFRL",     .hwaddr = 0x20, .initValue = 0x0, .regOffs = offsetof(STM32F446GpioState, afrl), .log = true},
	{.name = "AFRH",     .hwaddr = 0x24, .initValue = 0x0, .regOffs = offsetof(STM32F446GpioState, afrh), .log = true},
};

static const DeviceInfo deviceInfo = {
	.regInfoSize = arraySize(regInfo),
	.regInfo = regInfo
};

static void stm32f446_gpio_reset(DeviceState *dev)
{
  STM32F446GpioState *s = STM32F446_GPIO(dev);
  stm32f445_util_reset(deviceInfo, (char *)s);
}

static uint64_t stm32f446_gpio_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
  STM32F446GpioState *s = opaque;
  return stm32f445_util_regRead(deviceInfo, s->name, (char *) opaque, addr, size);
}

static void stm32f446_gpio_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
	STM32F446GpioState *s = opaque;
  stm32f445_util_regWrite(deviceInfo, s->name, (char *) opaque, addr, val64, size);
}

static const MemoryRegionOps stm32f446_gpio_ops = {
    .read = stm32f446_gpio_read,
    .write = stm32f446_gpio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f446_gpio_init(Object *obj)
{
    STM32F446GpioState *s = STM32F446_GPIO(obj);
    memory_region_init_io(&s->mmio, obj, &stm32f446_gpio_ops, s, TYPE_STM32F446_GPIO, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static const VMStateDescription vmstate_stm32f446_gpio = {
		.name = TYPE_STM32F446_GPIO, .version_id = 1, .minimum_version_id = 1, .fields = (VMStateField[])
		{
		  VMSTATE_UINT32(moder, STM32F446GpioState),
			VMSTATE_UINT32(otyper, STM32F446GpioState),
		  VMSTATE_UINT32(ospeeder, STM32F446GpioState),
			VMSTATE_UINT32(puprd, STM32F446GpioState),
		  VMSTATE_UINT32(idr, STM32F446GpioState),
		  VMSTATE_UINT32(odr, STM32F446GpioState),
		  VMSTATE_UINT32(bsrr, STM32F446GpioState),
		  VMSTATE_UINT32(lckr, STM32F446GpioState),
		  VMSTATE_UINT32(afrl, STM32F446GpioState),
		  VMSTATE_UINT32(afrh, STM32F446GpioState),
			VMSTATE_END_OF_LIST()
		}
};

static Property stm32f446_gpio_properties[] = {
    DEFINE_PROP_STRING("name", STM32F446GpioState, name),
    DEFINE_PROP_END_OF_LIST(),
};

static void stm32f446_gpio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f446_gpio_reset;
    dc->vmsd = &vmstate_stm32f446_gpio;
    device_class_set_props(dc, stm32f446_gpio_properties);
}

static const TypeInfo stm32f446_gpio_info = {
    .name          = TYPE_STM32F446_GPIO,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F446GpioState),
    .instance_init = stm32f446_gpio_init,
    .class_init    = stm32f446_gpio_class_init,
};

static void stm32f446_gpio_register_types(void)
{
    type_register_static(&stm32f446_gpio_info);
}

type_init(stm32f446_gpio_register_types)
