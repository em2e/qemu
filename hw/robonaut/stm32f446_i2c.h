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

#ifndef HW_STM32F446_I2C_H
#define HW_STM32F446_I2C_H

#include "hw/sysbus.h"
#include "hw/irq.h"
#include "qom/object.h"

#define TYPE_STM32F446_I2C "stm32f446-i2c"
OBJECT_DECLARE_SIMPLE_TYPE(STM32F446I2cState, STM32F446_I2C)

struct STM32F446State;
typedef void (*I2cCallback) (STM32F446I2cState *i2c);
typedef void (*I2cInputReady)(STM32F446I2cState *i2c, size_t size);


enum STM32F446I2cMode {STM32F446I2cMODE_MASTERTRANSMIT = 2, STM32F446I2cMODE_MASTERRECEIVE = 3, STM32F446I2cMODE_SLAVERANSMIT = 4, STM32F446I2cMODE_SLAVERECEIVE = 5};

struct STM32F446I2cState {
  /* <private> */
  SysBusDevice parent_obj;

  /* <public> */
  MemoryRegion mmio;
  qemu_irq irq;

  struct STM32F446State *soc;
  I2cCallback callback;
  I2cInputReady inputReady;

  char *name;
  enum STM32F446I2cMode mode;
  uint32_t clientAddr;
  uint8_t inpBuffer[256];
  uint32_t inpSize;
  uint32_t inpIndex;
  uint8_t outBuffer[256];
  uint32_t outIndex;
  bool sr1Read;

  uint32_t cr1;
  uint32_t cr2;
  uint32_t oar1;
  uint32_t oar2;
  uint32_t dr;
  uint32_t sr1;
  uint32_t sr2;
  uint32_t ccr;
  uint32_t trise;
  uint32_t fltr;
};

#endif /* HW_STM32F446_I2C_H */

