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
#include "sysemu/runstate.h"
#include <hw/robonaut/stm32f446_soc.h>

#include <hw/robonaut/simulatorConnection.h>

/* Main SYSCLK frequency in Hz (180MHz) */
#define SYSCLK_FRQ 180000000ULL
//HCLK (AHB) 168Mhz,  180Mhz in overdrive mode
//PCLK1 (APB1) 42, 45
//PCLK2 (APB2) 84, 90
//#define SYSCLK_INIT_FRQ 16000000ULL //init fq of HSI: 16Mhz

static SimulatorConnectionState simulatorConnection;

#define SERVO_BACK_MID  11000 // mid 11000
#define SERVO_BACK_HIGH 13500
#define SERVO_BACK_LOW  8000 // mid 17500
#define SERVO_FRONT_MID 9000 // mid 9000
#define SERVO_FRONT_HIGH  12000 // mid 17500
#define SERVO_FRONT_LOW 6000
#define TAVSERVO_MID 14141
#define TAVSERVO_LOW 11447
#define TAVSERVO_HIGH 16834

static double pwmToAngle(uint32_t pwm, uint32_t low, uint32_t mid, uint32_t high) {
	return (pwm < mid)
			? (double) (mid - pwm) / (double) (mid - low) * -32.5
			: (double) (pwm - mid) / (double) (high - mid) * 32.5;
}

static void robonaut_I2c2Callback(STM32F446I2cState *i2c) {
	STM32F446State *soc = i2c->soc;
	simulatorConnection.outMsg.virtualTime = qemu_clock_get_ns (QEMU_CLOCK_VIRTUAL);
	i2c->outBuffer[i2c->outIndex] = 0;
	simulatorConnection.outMsg.motorPower = atof((const char *)i2c->outBuffer);
	i2c->outBuffer[0] = 0;
	i2c->outIndex = 0;

	simulatorConnection.outMsg.fwdSteeringWheelAngle = pwmToAngle(soc->timer[2].tim_ccr1, SERVO_FRONT_LOW, SERVO_FRONT_MID, SERVO_FRONT_HIGH);
	simulatorConnection.outMsg.revSteeringWheelAngle = pwmToAngle(soc->timer[3].tim_ccr1, SERVO_BACK_LOW, SERVO_BACK_MID, SERVO_BACK_HIGH);
	simulatorConnection.outMsg.distanceRotationAngle = pwmToAngle(soc->timer[4].tim_ccr1, TAVSERVO_LOW, TAVSERVO_MID, TAVSERVO_HIGH);

	simulatorConnection_signalOutThread(&simulatorConnection);

}

#define MSG_STATE_SHUTDOWN 0u
#define MSG_STATE_RUNNING 1u
#define MSG_STATE_WAITING 2u

static void robonaut_I2cInputCallback(void *opaque,size_t size,  SimulatoConnectionInputMessage *msg)
{
	STM32F446State *soc = opaque;
	if (size < 1) {
		//NULL msg?
		return;
	}

	if (msg->state == MSG_STATE_SHUTDOWN)
	{
		qemu_system_shutdown_request(SHUTDOWN_CAUSE_GUEST_SHUTDOWN);
	}
	else if (msg->state == MSG_STATE_WAITING)
	{
		//do nothing
	}
	else if (msg->state == MSG_STATE_RUNNING && size >= sizeof(SimulatoConnectionInputMessage))
	{
		sprintf((char *) soc->i2c_2.inpBuffer, "%c,%c,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f",
				msg->morelines ? '1' : '0',
				msg->oneline ? '1' : '0',
				msg->pFront1, msg->pFront2, msg->pFront3, msg->pFront4,
				msg->pMid1, msg->pMid2, msg->pMid3, msg->pMid4);

		printf("Input buffer: \"%s\"\n", soc->i2c_2.inpBuffer);
		soc->i2c_2.inputReady(&soc->i2c_2, 90);

		int64_t currentCnt = (int64_t) soc->timer[1].tim_cnt;
		if (msg->encoder >= 0) {
			currentCnt += msg->encoder;
			if (soc->timer[1].tim_arr) {
				while(currentCnt > soc->timer[1].tim_arr) {
					currentCnt -= soc->timer[1].tim_arr;
				}
			}
		} else {
			currentCnt += msg->encoder;
			if (soc->timer[1].tim_arr) {
				while(currentCnt < 0) {
					currentCnt += soc->timer[1].tim_arr;
				}
			}
		}
		soc->timer[1].tim_cnt = currentCnt & 0xffffffff;
		printf("Speed: msg.encoder=%ld, currentCnt=%ld, tim_cnt=%u\n", msg->encoder, currentCnt, soc->timer[1].tim_cnt);
	}
}

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

  simulatorConnection.inpCallbackParam = dev;
  STM32F446State *soc = STM32F446_SOC(dev);
  soc->i2c_2.callback = robonaut_I2c2Callback;
  simulatorConnection.inpCallback = robonaut_I2cInputCallback;
}

static void robonaut_machine_init (MachineClass *mc)
{
  mc->desc = "Robonaut emulated hardware";
  mc->init = robonaut_init;
}



DEFINE_MACHINE("robonaut", robonaut_machine_init)
