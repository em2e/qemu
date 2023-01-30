/*
 * Robonaut machine model
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
#include "qapi/error.h"
#include "hw/boards.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-clock.h"
#include "qemu/error-report.h"
#include "hw/arm/boot.h"
#include <hw/robonaut/stm32f446_soc.h>

#include <hw/robonaut/simulatorConnection.h>

/* Main SYSCLK frequency in Hz (180MHz) */
#define SYSCLK_FRQ 180000000ULL
//HCLK (AHB) 168Mhz,  180Mhz in overdrive mode
//PCLK1 (APB1) 42, 45
//PCLK2 (APB2) 84, 90
//#define SYSCLK_INIT_FRQ 16000000ULL //init fq of HSI: 16Mhz

static SimulatorConnectionState simulatorConnection;

static void robonaut_init (MachineState *machine)
{
  DeviceState *dev;
  Clock *sysclk;

  if (!simulatorConnection_init (&simulatorConnection))
  {
      error_setg(&error_fatal, "Socket connection failed");
      return;
  }

  sysclk = clock_new (OBJECT(machine), "SYSCLK");
  clock_set_hz (sysclk, SYSCLK_FRQ); //initially the system clock is the internal 16Mhz clock, but our firmware will switch to the maximum 180Mhz using HSE source

  dev = qdev_new (TYPE_STM32F446RE_SOC);
  qdev_prop_set_string (dev, "cpu-type", ARM_CPU_TYPE_NAME("cortex-m4"));
  qdev_connect_clock_in (dev, "sysclk", sysclk);
  sysbus_realize_and_unref (SYS_BUS_DEVICE (dev), &error_fatal);

  armv7m_load_kernel (ARM_CPU (first_cpu), machine->kernel_filename, 0, FLASH_SIZE_E);
}

static void robonaut_machine_init (MachineClass *mc)
{
  mc->desc = "Robonaut emulated hardware";
  mc->init = robonaut_init;
}

DEFINE_MACHINE("robonaut", robonaut_machine_init)
