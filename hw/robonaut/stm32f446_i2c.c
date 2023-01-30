/*
 * STM32F446 I2C
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
#include <hw/robonaut/stm32f446_util.h>
#include <hw/robonaut/stm32f446_i2c.h>
//#include <hw/robonaut/stm32f446_soc.h>

//	uint64_t hwaddr;
//	const char * name;
//	uint32_t initValue;
//	uint32_t *reg;

static const RegisterInfo regInfo[] = {
	{.name = "CR1", .hwaddr = 0x0, .initValue = 0x0, .regOffs = offsetof(STM32F446I2cState, cr1)},
	{.name = "CR2",	.hwaddr = 0x04, .initValue = 0x0, .regOffs = offsetof(STM32F446I2cState, cr2)}
};

static const DeviceInfo deviceInfo = {
	.name = "I2C",
	.regCount = sizeof(regInfo)/sizeof(regInfo[0]),
	.regInfo = regInfo
};

static void stm32f446_i2c_reset(DeviceState *dev)
{
  stm32f445_util_reset(deviceInfo, (char *)dev);
}

static uint64_t stm32f446_i2c_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
  return stm32f445_util_read(deviceInfo, (char *) opaque, addr, size);
}

static void stm32f446_i2c_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{

	stm32f445_util_write(deviceInfo, (char *) opaque, addr, val64 size);

	STM32F446I2cState *s = opaque;
    uint32_t value = val64;
}

static const MemoryRegionOps stm32f446_i2c_ops = {
    .read = stm32f446_i2c_read,
    .write = stm32f446_i2c_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void stm32f446_i2c_init(Object *obj)
{
    STM32F446I2cState *s = STM32F446_I2C(obj);

    memory_region_init_io(&s->mmio, obj, &stm32f446_i2c_ops, s, TYPE_STM32F446_I2C, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->mmio);
}

static const VMStateDescription vmstate_stm32f446_i2c = {
    .name = TYPE_STM32F446_I2C,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(cr1, STM32F446I2cState),
        VMSTATE_UINT32(cr2, STM32F446I2cState),
        VMSTATE_END_OF_LIST()
    }
};

static void stm32f446_i2c_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f446_i2c_reset;
    dc->vmsd = &vmstate_stm32f446_i2c;
}

static const TypeInfo stm32f446_i2c_info = {
    .name          = TYPE_STM32F446_I2C,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(STM32F446I2cState),
    .instance_init = stm32f446_i2c_init,
    .class_init    = stm32f446_i2c_class_init,
};

static void stm32f446_i2c_register_types(void)
{
    type_register_static(&stm32f446_i2c_info);
}

type_init(stm32f446_i2c_register_types)
