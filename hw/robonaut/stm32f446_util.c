/*
 * STM32F446 COMMON UTILITY
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
#include "hw/qdev-clock.h"
#include "trace.h"
#include <hw/robonaut/stm32f446_util.h>

void stm32f446_util_unhandledRead(const char *deviceName, hwaddr addr,
                                     unsigned int size)
{
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Bad offset 0x%"HWADDR_PRIx"\n", deviceName, addr);
}

bool stm32f446_util_regBitChange(uint32_t value, uint32_t *ch, const char *devName, const char *regName,const char *bitName, uint32_t bit_bb)
{
  uint32_t mask = (1u << bit_bb);
  if (*ch & mask) {
    *ch &= ~mask;
    bool b = value & mask;
    qemu_log_mask(LOG_GUEST_ERROR, "%s: %s %s: %d\n", devName, regName, bitName, b);
    return true;
  }
  return false;
}

void stm32f446_util_regUnhandled(uint32_t value, uint32_t *ch, const char *devName, const char *regName)
{
  if (*ch > 0)
  {
    qemu_log_mask(LOG_GUEST_ERROR, "%s: %s UNHANDLED change: 0x%0x, value: 0x%0x\n", devName, regName, *ch, value);
  }
}

//clock handling logic copied from clock.c because the original code does not support updating the multiplier / divider properties of an already connected clock
static void clockCallCallback(Clock *clk, ClockEvent event)
{
    if (clk->callback && (clk->callback_events & event)) {
        clk->callback(clk->callback_opaque, event);
    }
}

static uint64_t clockGetChildPeriod(Clock *clk)
{
    return muldiv64(clk->period, clk->multiplier, clk->divider);
}

static void clockPropagatePeriod(Clock *clk, bool callCallbacks)
{
    Clock *child;
    uint64_t child_period = clockGetChildPeriod(clk);

    QLIST_FOREACH(child, &clk->children, sibling) {
        if (child->period != child_period) {
            if (callCallbacks) {
              clockCallCallback(child, ClockPreUpdate);
            }
            child->period = child_period;
            if (callCallbacks) {
              clockCallCallback(child, ClockUpdate);
            }
            clockPropagatePeriod(child, callCallbacks);
        }
    }
}


void stm32f446_util_clockUpdateMulDiv(Clock *clk, uint32_t multiplier, uint32_t divider, bool propagate)
{
    assert(divider != 0);
    assert(multiplier != 0);

    clk->multiplier = multiplier;
    clk->divider = divider;

    if (propagate) {
      clockPropagatePeriod(clk, true);
    }
}

uint32_t stm32f446_util_clockGetChildHz(Clock *clk)
{
  return CLOCK_PERIOD_TO_HZ(clockGetChildPeriod(clk));
}
