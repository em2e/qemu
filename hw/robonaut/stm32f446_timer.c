/*
 * STM32F446 Timer
 *
 * based on STM32F2XX Timer by Alistair Francis <alistair@alistair23.me>
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
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include <hw/robonaut/stm32f446_timer.h>
#include <hw/robonaut/stm32f446_util.h>
#include <inttypes.h>

#ifndef STM_TIMER_ERR_DEBUG
#define STM_TIMER_ERR_DEBUG 1
#endif

#define DB_PRINT_L(lvl, fmt, args...) do { \
    if (STM_TIMER_ERR_DEBUG >= lvl) { \
        qemu_log("%s: " fmt, __func__, ## args); \
    } \
} while (0)

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ## args)

#define isEnabled(s) ((s->tim_cr1 & STM32F446_TIMER_CEN) > 0)

static void stm32f446_timer_set_alarm (STM32F446TimerState *s);

static const RegisterInfo regInfo[] = {
		{ .name = "CR1", .hwaddr = TIM_CR1, .regOffs = offsetof(STM32F446TimerState, tim_cr1), .log = true },
		{ .name = "CR2", .hwaddr = TIM_CR2, .regOffs = offsetof(STM32F446TimerState, tim_cr2), .log = true },
		{ .name = "SMCR", .hwaddr = TIM_SMCR, .regOffs = offsetof(STM32F446TimerState, tim_smcr), .log = true },
		{ .name = "DIER", .hwaddr = TIM_DIER, .regOffs = offsetof(STM32F446TimerState, tim_dier), .log = true },
		{ .name = "SR", .hwaddr = TIM_SR, .regOffs = offsetof(STM32F446TimerState, tim_sr), .log = true },
		{ .name = "EGR", .hwaddr = TIM_EGR, .regOffs = offsetof(STM32F446TimerState, tim_egr), .log = true },
		{ .name = "CCMR1", .hwaddr = TIM_CCMR1, .regOffs = offsetof(STM32F446TimerState, tim_ccmr1), .log = true },
		{ .name =	"CCMR2", .hwaddr = TIM_CCMR2, .regOffs = offsetof(STM32F446TimerState, tim_ccmr2), .log = true },
		{ .name = "CCER", .hwaddr = TIM_CCER, .regOffs = offsetof(STM32F446TimerState, tim_ccer), .log = true },
		{ .name = "CNT", .hwaddr = TIM_CCER, .regOffs = offsetof(STM32F446TimerState, tim_cnt), .log = true },
		{ .name = "PSC", .hwaddr = TIM_PSC, .regOffs = offsetof(STM32F446TimerState, tim_psc), .log = true },
		{ .name = "ARR", .hwaddr = TIM_ARR, .regOffs = offsetof(STM32F446TimerState, tim_arr), .log = true },
		{ .name = "RCR", .hwaddr = TIM_RCR, .regOffs = offsetof(STM32F446TimerState, tim_rcr), .log = true },
		{ .name = "CCR1", .hwaddr = TIM_CCR1, .regOffs = offsetof(STM32F446TimerState, tim_ccr1), .log = false },
		{ .name = "CCR2", .hwaddr = TIM_CCR2, .regOffs = offsetof(STM32F446TimerState, tim_ccr2), .log = true },
		{ .name = "CCR3", .hwaddr = TIM_CCR3, .regOffs = offsetof(STM32F446TimerState, tim_ccr3),.log = true },
		{ .name = "CCR4", .hwaddr = TIM_CCR4, .regOffs = offsetof(STM32F446TimerState, tim_ccr4), .log = true },
		{ .name = "BDTR", .hwaddr = TIM_BDTR, .regOffs = offsetof(STM32F446TimerState, tim_bdtr), .log = true },
		{ .name = "DCR", .hwaddr = TIM_DCR, .regOffs = offsetof(STM32F446TimerState, tim_dcr), .log = true },
		{ .name = "DMAR", .hwaddr = TIM_DMAR, .regOffs = offsetof(STM32F446TimerState, tim_dmar), .log = true },
		{ .name = "OR", .hwaddr = TIM_OR, .regOffs = offsetof(STM32F446TimerState, tim_or), .log = true }
};

static const DeviceInfo deviceInfo = { .regInfoSize = arraySize (regInfo), .regInfo = regInfo };

static void stm32f446_timer_interrupt (void *opaque) {
	STM32F446TimerState *s = opaque;

	//DB_PRINT("Interrupt\n");

	if ((s->tim_dier & TIM_DIER_UIE) && (s->tim_cr1 & STM32F446_TIMER_CEN))
	{
		s->tim_sr |= 1;
		qemu_irq_pulse (s->irq);
		stm32f446_timer_set_alarm (s);
	}

	/*   if (s->tim_ccmr1 & (TIM_CCMR1_OC2M2 | TIM_CCMR1_OC2M1) &&
	 !(s->tim_ccmr1 & TIM_CCMR1_OC2M0) &&
	 s->tim_ccmr1 & TIM_CCMR1_OC2PE &&
	 s->tim_ccer & TIM_CCER_CC2E) {
	 DB_PRINT("PWM2 Duty Cycle: %d%%\n",
	 s->tim_ccr2 / (100 * (s->tim_psc + 1)));
	 }
	 */
}

static void stm32f446_timer_enableChanged (STM32F446TimerState *s) {
	qemu_log_mask(LOG_GUEST_ERROR, "%s: enabled/disabled: 0x%0x\n", s->name, s->tim_cr1);
	stm32f446_timer_set_alarm (s);
	if (s->ticks > 0)
	{
		qemu_log_mask(LOG_GUEST_ERROR, "%s: alarm set in %ld ticks, %ld ns\n", s->name, s->ticks, clock_ticks_to_ns (s->clk, s->ticks));
	}
}

static void stm32f446_timer_set_alarm (STM32F446TimerState *s) {
	//only upcounting
	s->tim_cnt = 0;
	if (!isEnabled(s) || s->tim_arr == 0)
	{
		s->ticks = 0;
		return;
	}

	uint64_t nowNs = qemu_clock_get_ns (QEMU_CLOCK_VIRTUAL);
	s->ticks = (s->tim_arr + 1) * (s->tim_psc + 1) * (s->tim_rcr + 1);

	timer_mod (s->timer, nowNs + clock_ticks_to_ns (s->clk, s->ticks));
	s->startTimeNs = nowNs;
	/*
	 qemu_log_mask(LOG_GUEST_ERROR, "TIMER[%d]: alarm set in %ld ticks\n",
	 s->timerId, s->ticks);
	 */
}

static void stm32f446_timer_reset (DeviceState *dev) {
	STM32F446TimerState *s = STM32F446TIMER (dev);
	stm32f445_util_reset (deviceInfo, (char*) s);
}

static uint64_t stm32f446_timer_getCnt (STM32F446TimerState *s) {
	if (s->ticks > 0)
	{
		uint64_t nowTicks = clock_ns_to_ticks (s->clk, qemu_clock_get_ns (QEMU_CLOCK_VIRTUAL) - s->startTimeNs);
		if (nowTicks < s->ticks)
		{
			s->tim_cnt = nowTicks;
		}
	}
	return s->tim_cnt;
}

static uint64_t stm32f446_timer_read (void *opaque, hwaddr offset, unsigned size) {
	STM32F446TimerState *s = opaque;
	/*
	 qemu_log_mask(LOG_GUEST_ERROR, "TIMER[%d]: read (size %d, addr 0x%0*" HWADDR_PRIx ")\n",
	 s->timerId, size, 4, offset);
	 */
	switch (offset)
	{
		case TIM_CNT:
			return stm32f446_timer_getCnt (s);
		default:
			return stm32f445_util_regRead (deviceInfo, s->name, (char*) opaque, offset, size);
	}

	return 0;
}

static void stm32f446_timer_write (void *opaque, hwaddr offset, uint64_t val64, unsigned size) {
	STM32F446TimerState *s = opaque;
	uint32_t value = val64;
	/*
	 qemu_log_mask(LOG_GUEST_ERROR, "TIMER[%d]: write (size %d, offset 0x%0*" HWADDR_PRIx ", value 0x%0*" PRIx64 ")\n",
	 s->timerId, size, 4, offset, size << 1, val64);
	 */
	switch (offset)
	{
		case TIM_CR1:
		{
			bool cenChanged = ((s->tim_cr1 & 1) != (value & 1));
			s->tim_cr1 = value;
			if (cenChanged)
			{
				stm32f446_timer_enableChanged (s);
			}
		}
			return;
		case TIM_SR:
			/* This is set by hardware and cleared by software */
			s->tim_sr &= value;
			return;
		case TIM_EGR:
			s->tim_egr = value;
			if (s->tim_egr & TIM_EGR_UG)
			{
				break;
			}
			return;
		default:
		  stm32f445_util_regWrite(deviceInfo, s->name, (char *) opaque, offset, val64, size);
			return;
	}

	/* This means that a register write has affected the timer in a way that
	 * requires a refresh of both tick_offset and the alarm.
	 */
	stm32f446_timer_set_alarm (s);
}

static const MemoryRegionOps stm32f446_timer_ops = { .read = stm32f446_timer_read, .write = stm32f446_timer_write, .endianness = DEVICE_NATIVE_ENDIAN, };

static const VMStateDescription vmstate_stm32f446_timer = { .name = TYPE_STM32F446_TIMER, .version_id = 1, .minimum_version_id = 1, .fields = (VMStateField[] ) {
		VMSTATE_UINT32(tim_cr1, STM32F446TimerState),
		VMSTATE_UINT32(tim_cr2, STM32F446TimerState),
		VMSTATE_UINT32(tim_smcr, STM32F446TimerState),
		VMSTATE_UINT32(tim_dier, STM32F446TimerState),
		VMSTATE_UINT32(tim_sr, STM32F446TimerState),
		VMSTATE_UINT32(tim_egr, STM32F446TimerState),
		VMSTATE_UINT32(tim_ccmr1, STM32F446TimerState),
		VMSTATE_UINT32(tim_ccmr2, STM32F446TimerState),
		VMSTATE_UINT32(tim_ccer, STM32F446TimerState),
		VMSTATE_UINT32(tim_psc, STM32F446TimerState),
		VMSTATE_UINT32(tim_arr, STM32F446TimerState),
		VMSTATE_UINT32(tim_rcr, STM32F446TimerState),
		VMSTATE_UINT32(tim_ccr1, STM32F446TimerState),
		VMSTATE_UINT32(tim_ccr2, STM32F446TimerState),
		VMSTATE_UINT32(tim_ccr3, STM32F446TimerState),
		VMSTATE_UINT32(tim_ccr4, STM32F446TimerState),
		VMSTATE_UINT32(tim_bdtr, STM32F446TimerState),
		VMSTATE_UINT32(tim_dcr, STM32F446TimerState),
		VMSTATE_UINT32(tim_dmar, STM32F446TimerState),
		VMSTATE_UINT32(tim_or, STM32F446TimerState),
		VMSTATE_END_OF_LIST() } };

static Property stm32f446_timer_properties[] = {
		DEFINE_PROP_STRING("name", struct STM32F446TimerState, name),
		//DEFINE_PROP_UINT32("timerId", struct STM32F446TimerState, timerId, 0),
		DEFINE_PROP_UINT32("timerType", struct STM32F446TimerState, timerType, STM32F446_TIMER_TYPE_GENERALPURPOSE),
		DEFINE_PROP_END_OF_LIST(), };

static void stm32f446_timer_clockUpdate (void *opaque, ClockEvent event) {
	STM32F446TimerState *s = STM32F446TIMER (opaque);
	qemu_log_mask(LOG_GUEST_ERROR, "%s: clock source update event: %d\n", s->name, event);

	if (s->ticks > 0)
	{
		if (event == ClockPreUpdate)
		{
			stm32f446_timer_getCnt (s); //get current ticks
		}
		else if (event == ClockUpdate)
		{
			uint64_t nowNs = qemu_clock_get_ns (QEMU_CLOCK_VIRTUAL);
			timer_mod (s->timer, nowNs + clock_ticks_to_ns (s->clk, s->ticks - s->tim_cnt));
		}
	}
}

static void stm32f446_timer_init (Object *obj) {
	STM32F446TimerState *s = STM32F446TIMER (obj);

	sysbus_init_irq (SYS_BUS_DEVICE (obj), &s->irq);

	memory_region_init_io (&s->iomem, obj, &stm32f446_timer_ops, s, "stm32f446_timer", 0x400);
	sysbus_init_mmio (SYS_BUS_DEVICE (obj), &s->iomem);

	s->clk = qdev_init_clock_in (DEVICE (s), "clk", stm32f446_timer_clockUpdate, s, ClockPreUpdate | ClockUpdate);

}

static void stm32f446_timer_realize (DeviceState *dev, Error **errp) {
	STM32F446TimerState *s = STM32F446TIMER (dev);
	s->timer = timer_new_ns (QEMU_CLOCK_VIRTUAL, stm32f446_timer_interrupt, s);
}

static void stm32f446_timer_class_init (ObjectClass *klass, void *data) {
	DeviceClass *dc = DEVICE_CLASS (klass);

	dc->reset = stm32f446_timer_reset;
	device_class_set_props (dc, stm32f446_timer_properties);
	dc->vmsd = &vmstate_stm32f446_timer;
	dc->realize = stm32f446_timer_realize;
}

static const TypeInfo stm32f446_timer_info = { .name = TYPE_STM32F446_TIMER, .parent = TYPE_SYS_BUS_DEVICE, .instance_size = sizeof(STM32F446TimerState), .instance_init = stm32f446_timer_init,
		.class_init = stm32f446_timer_class_init, };

static void stm32f446_timer_register_types (void) {
	type_register_static (&stm32f446_timer_info);
}

type_init(stm32f446_timer_register_types)
