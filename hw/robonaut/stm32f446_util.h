/*
 * STM32F446 UTIL
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

#ifndef HW_STM32F446_UTIL_H
#define HW_STM32F446_UTIL_H

#include <stdbool.h>
#include "hw/sysbus.h"
#include "hw/qdev-clock.h"

void stm32f446_util_unhandledRead(const char *deviceName, hwaddr addr, unsigned int size);
bool stm32f446_util_regBitChange(uint32_t value, uint32_t *ch, const char *devName, const char *regName,const char *bitName, uint32_t bit_bb);
void stm32f446_util_regUnhandled(uint32_t value, uint32_t *ch, const char *devName, const char *regName);
void stm32f446_util_clockUpdateMulDiv(Clock *clk, uint32_t multiplier, uint32_t divider, bool propagate);
uint32_t stm32f446_util_clockGetChildHz(Clock *clk);

#endif /* HW_STM32F446_UTIL_H */

