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

#ifndef HW_STM32F446_GPIO_H
#define HW_STM32F446_GPIO_H

#include "hw/sysbus.h"
#include "qom/object.h"

#define TYPE_STM32F446_GPIO "stm32f446-gpio"
OBJECT_DECLARE_SIMPLE_TYPE(STM32F446GpioState, STM32F446_GPIO)

struct STM32F446State;

typedef void (*GpioCallback) (STM32F446GpioState *gpio);

struct STM32F446GpioState {
  /* <private> */
  SysBusDevice parent_obj;

  /* <public> */
  MemoryRegion mmio;

  struct STM32F446State *soc;
  GpioCallback callback;

  char *name;

  uint32_t moder;
  uint32_t otyper;
  uint32_t ospeeder;
  uint32_t puprd;
	uint32_t idr;
  uint32_t odr;
  uint32_t bsrr;
  uint32_t lckr;
  uint32_t afrl;
  uint32_t afrh;
  };

#endif /* HW_STM32F446_GPIO_H */

