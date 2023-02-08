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
#include "hw/irq.h"
#include "trace.h"
#include "migration/vmstate.h"
#include "hw/qdev-properties.h"
#include <hw/robonaut/stm32f446_util.h>
#include <hw/robonaut/stm32f446_i2c.h>
//#include <hw/robonaut/stm32f446_soc.h>

//	uint64_t hwaddr;
//	const char * name;
//	uint32_t initValue;
//	uint32_t *reg;

#define CR1_START (1u << 8)
#define CR1_STOP (1u << 9)
#define CR1_SWRST (1u << 15)
#define CR2_ITBUFEN (1u << 10)
#define CR2_ITEVTEN (1u << 9)
#define SR1_SB 1u
#define SR1_ADDR 2u
#define SR1_TXE (1u << 7)
#define SR1_RXNE (1u << 6)
#define SR1_BTF (1u << 2)
#define SR2_MSL 1u
#define SR2_BUSY 2u
#define SR2_TRA 4u


static void cr1StartWriteHandler(void *device, uint32_t *ch, uint32_t *value)
{
  STM32F446I2cState *s = device;
  if (*value & CR1_START) {
  	s->sr1 &= ~(SR1_ADDR | SR1_TXE);
  	s->sr1 |= SR1_SB; //Start condition generated
  	s->sr2 |= (SR2_MSL | SR2_BUSY); //master mode & busy
  	s->outIndex = 0;
  	s->inpIndex = 0;
  	s->mode = 0;
  	if (s->cr2 & CR2_ITEVTEN) {
  		qemu_irq_pulse (s->irq);
  	}
  }
}

static void cr1StopWriteHandler(void *device, uint32_t *ch, uint32_t *value)
{
  STM32F446I2cState *s = device;
  if (*value & CR1_STOP) {
  	if (s->callback != NULL) {
  		s->callback(s);
  	}
  	s->sr2 &= ~(SR2_MSL | SR2_BUSY | SR2_TRA); //master mode & busy
  	s->sr1 &= ~(SR1_ADDR | SR1_TXE | SR1_BTF);
  	s->mode = 0;
  }
}

static uint64_t drReadHandler(void *device)
{
  STM32F446I2cState *s = device;
  uint32_t dr = s->dr;
  if ((s->inpSize > s->inpIndex) && (s->mode == STM32F446I2cMODE_MASTERRECEIVE) && (s->cr2 & CR2_ITEVTEN)) {
		s->dr = s->inpBuffer[s->inpIndex];
		s->inpIndex++;
		s->sr1 |= (SR1_RXNE | SR1_BTF);
		qemu_irq_pulse (s->irq);
	}
  return dr;
}

static void drWriteHandler(void *device, uint32_t *ch, uint32_t *value)
{
  STM32F446I2cState *s = device;
  if (s->cr1 & CR1_START) {
  	if (*value & 1u)
  	{
  		//receive
  		s->mode = STM32F446I2cMODE_MASTERRECEIVE;

  		s->sr1 &= ~SR1_SB; //Start condition cleared
      s->sr1 |= SR1_ADDR;
      s->sr2 &= ~SR2_TRA;
      s->cr1 &= ~CR1_START;
      s->clientAddr = *value;

  	}
  	else
  	{
  		//transmit
  		s->mode = STM32F446I2cMODE_MASTERTRANSMIT;

  		s->sr1 &= ~SR1_SB; //Start condition cleared
      s->sr1 |= (SR1_ADDR | SR1_TXE);
      s->sr2 |= SR2_TRA;
      s->cr1 &= ~(CR1_START);
      s->clientAddr = *value;

  	}
  	if (s->cr2 & CR2_ITEVTEN) {
  		qemu_irq_pulse (s->irq);
  	}
  } else {
  	if (s->outIndex > arraySize(s->outBuffer)) {
  		//overflow!!!!
  	}	else {
			s->outBuffer[s->outIndex] = (uint8_t) *value;
			s->outIndex++;
  	}
    s->sr1 |= (SR1_TXE | SR1_BTF);
  	if (s->cr2 & CR2_ITEVTEN) {
  		qemu_irq_pulse (s->irq);
  	}
  }
  s->dr = *value;
  *ch = 0;
  s->sr1Read = false;
}

static void cr1SwrstWriteHandler(void *device, uint32_t *ch, uint32_t *value)
{
  STM32F446I2cState *s = device;
  if (*value & CR1_SWRST) {
  	//reset
  	s->sr2 &= ~(SR2_MSL | SR2_BUSY | SR2_TRA);
  	s->sr1 &= ~(SR1_ADDR | SR1_TXE | SR1_BTF);
  	s->mode = 0;
  }
}

static uint64_t sr1ReadHandler(void *device) {
  STM32F446I2cState *s = device;
  s->sr1Read = true;
  return s->sr1;
}

static void inputReady(STM32F446I2cState *s, size_t size) {
	s->inpSize = size;
	s->inpIndex = 0;
	if ((s->mode == STM32F446I2cMODE_MASTERRECEIVE) && (s->cr2 & CR2_ITEVTEN)
			&& ((s->sr1 & SR1_ADDR) == 0)) {

		printf("indulna a mandula1\n");

		s->dr = s->inpBuffer[s->inpIndex];
		s->inpIndex++;
		s->sr1 |= (SR1_RXNE | SR1_BTF);
		qemu_irq_pulse (s->irq); //innen jön a hiba

	}
}

static uint64_t sr2ReadHandler(void *device) {
  STM32F446I2cState *s = device;
  if (s->sr1Read) {
  	s->sr1 &= ~(SR1_ADDR);
  	if ((s->mode == STM32F446I2cMODE_MASTERTRANSMIT) && (s->cr2 & CR2_ITEVTEN))
  	{
  		s->sr1 |= SR1_TXE;
  		qemu_irq_pulse (s->irq);
  	}
  	else if (((s->sr1 & SR1_RXNE) == 0) && (s->inpSize > s->inpIndex) && (s->mode == STM32F446I2cMODE_MASTERRECEIVE) && (s->cr2 & CR2_ITEVTEN))
  	{
  		printf("indul a mandula2\n");

  		s->dr = s->inpBuffer[s->inpIndex];
  		s->inpIndex++;
  		s->sr1 |= (SR1_RXNE | SR1_BTF);
  		qemu_irq_pulse (s->irq);
  	}
  }
  s->sr1Read = false;
  return s->sr2;
}

static const RegisterBitInfo cr1BitInfo[] = {
		{.name = "PE", .bb = 0, .mask = 1u, .log = true},
		{.name = "SMBUS", .bb = 1, .mask = 1u, .log = true},
		{.name = "SMBTYPE", .bb = 3, .mask = 1u, .log = true},
		{.name = "ENARP", .bb = 4, .mask = 1u, .log = true},
		{.name = "ENPEC", .bb = 5, .mask = 1u, .log = true},
		{.name = "ENGC", .bb = 6, .mask = 1u, .log = true},
		{.name = "NOSTRECH", .bb = 7, .mask = 1u, .log = true},
		{.name = "START", .bb = 8, .mask = 1u, .log = true, .wrHandler = cr1StartWriteHandler},
		{.name = "STOP", .bb = 9, .mask = 1u, .log = true, .wrHandler = cr1StopWriteHandler},
		{.name = "ACK", .bb = 10, .mask = 1u, .log = true},
		{.name = "POS", .bb = 11, .mask = 1u, .log = true},
		{.name = "PEC", .bb = 12, .mask = 1u, .log = true},
		{.name = "ALERT", .bb = 13, .mask = 1u, .log = true},
		{.name = "SWRST", .bb = 15, .mask = 1u, .log = true, .wrHandler = cr1SwrstWriteHandler},
};

static const RegisterInfo regInfo[] = {
	{.name = "CR1",   .hwaddr = 0x00, .initValue = 0x0, .regOffs = offsetof(STM32F446I2cState, cr1), .bitInfo = cr1BitInfo, .bitInfoSize = arraySize(cr1BitInfo) },
	{.name = "CR2",	  .hwaddr = 0x04, .initValue = 0x0, .regOffs = offsetof(STM32F446I2cState, cr2), .log = true},
	{.name = "OAR1",	.hwaddr = 0x08, .initValue = 0x0, .regOffs = offsetof(STM32F446I2cState, oar1), .log = true},
	{.name = "OAR2",	.hwaddr = 0x0c, .initValue = 0x0, .regOffs = offsetof(STM32F446I2cState, oar2), .log = true},
	{.name = "DR",    .hwaddr = 0x10, .initValue = 0x0, .regOffs = offsetof(STM32F446I2cState, dr), .log = true, .wrHandler = drWriteHandler, .rdHandler = drReadHandler},
	{.name = "SR1",   .hwaddr = 0x14, .initValue = 0x0, .regOffs = offsetof(STM32F446I2cState, sr1), .log = true, .rdHandler = sr1ReadHandler},
	{.name = "SR2",   .hwaddr = 0x18, .initValue = 0x0, .regOffs = offsetof(STM32F446I2cState, sr2), .log = true, .rdHandler = sr2ReadHandler, .readOnly = true},
	{.name = "CCR",   .hwaddr = 0x1c, .initValue = 0x0, .regOffs = offsetof(STM32F446I2cState, ccr), .log = true},
	{.name = "TRISE", .hwaddr = 0x20, .initValue = 0x2, .regOffs = offsetof(STM32F446I2cState, trise), .log = true},
	{.name = "FLTR",  .hwaddr = 0x24, .initValue = 0x0, .regOffs = offsetof(STM32F446I2cState, fltr), .log = true}
};

static const DeviceInfo deviceInfo = {
	.regInfoSize = arraySize(regInfo),
	.regInfo = regInfo
};

static void stm32f446_i2c_reset(DeviceState *dev)
{
  STM32F446I2cState *s = STM32F446_I2C(dev);
  stm32f445_util_reset(deviceInfo, (char *)s);
}

static uint64_t stm32f446_i2c_read(void *opaque, hwaddr addr,
                                     unsigned int size)
{
  STM32F446I2cState *s = opaque;
  return stm32f445_util_regRead(deviceInfo, s->name, (char *) opaque, addr, size);
}

static void stm32f446_i2c_write(void *opaque, hwaddr addr,
                       uint64_t val64, unsigned int size)
{
	STM32F446I2cState *s = opaque;
  stm32f445_util_regWrite(deviceInfo, s->name, (char *) opaque, addr, val64, size);
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
  	sysbus_init_irq (SYS_BUS_DEVICE (obj), &s->irq);
  	s->inputReady = inputReady;
}

static const VMStateDescription vmstate_stm32f446_i2c = {
		.name = TYPE_STM32F446_I2C, .version_id = 1, .minimum_version_id = 1, .fields = (VMStateField[])
		{
			VMSTATE_UINT32(cr1, STM32F446I2cState),
			VMSTATE_UINT32(cr2, STM32F446I2cState),
			VMSTATE_UINT32(oar1, STM32F446I2cState),
			VMSTATE_UINT32(oar2, STM32F446I2cState),
			VMSTATE_UINT32(dr, STM32F446I2cState),
			VMSTATE_UINT32(sr1, STM32F446I2cState),
			VMSTATE_UINT32(sr2, STM32F446I2cState),
			VMSTATE_UINT32(ccr, STM32F446I2cState),
			VMSTATE_UINT32(trise, STM32F446I2cState),
			VMSTATE_UINT32(fltr, STM32F446I2cState),
			VMSTATE_END_OF_LIST()
		}
};

static Property stm32f446_i2c_properties[] = {
    DEFINE_PROP_STRING("name", STM32F446I2cState, name),
    DEFINE_PROP_END_OF_LIST(),
};

static void stm32f446_i2c_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = stm32f446_i2c_reset;
    dc->vmsd = &vmstate_stm32f446_i2c;
    device_class_set_props(dc, stm32f446_i2c_properties);
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
