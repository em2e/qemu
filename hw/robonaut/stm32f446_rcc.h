/*
 * STM32F446 RCC
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

#ifndef HW_STM32F446_RCC_H
#define HW_STM32F446_RCC_H

#include "hw/sysbus.h"
#include "qom/object.h"

#define STM32F446_RCC_CR 0x00
#define STM32F446_RCC_PLLCFGR 0x04
#define STM32F446_RCC_CFGR 0x08
#define STM32F446_RCC_CIR 0x0c
#define STM32F446_RCC_AHB1RSTR 0x10
#define STM32F446_RCC_AHB2RSTR 0x14
#define STM32F446_RCC_AHB3RSTR 0x18
#define STM32F446_RCC_APB1RSTR 0x20
#define STM32F446_RCC_APB2RSTR 0x24
#define STM32F446_RCC_AHB1ENR 0x30
#define STM32F446_RCC_AHB2ENR 0x34
#define STM32F446_RCC_AHB3ENR 0x38
#define STM32F446_RCC_APB1ENR 0x40
#define STM32F446_RCC_APB2ENR 0x44
#define STM32F446_RCC_AHB1LPENR 0x50
#define STM32F446_RCC_AHB2LPENR 0x54
#define STM32F446_RCC_AHB3LPENR 0x58
#define STM32F446_RCC_APB1LPENR 0x60
#define STM32F446_RCC_APB2LPENR 0X64
#define STM32F446_RCC_BDCR 0x70
#define STM32F446_RCC_CSR 0x74
#define STM32F446_RCC_SSCGR 0x80
#define STM32F446_RCC_PLLI2SCFGR 0x84
#define STM32F446_RCC_PLLSAICFGR 0x88
#define STM32F446_RCC_DCKCFGR 0x80c
#define STM32F446_RCC_CKGATENR 0x90
#define STM32F446_RCC_DCKCFGR2 0x94

#define TYPE_STM32F446_RCC "stm32f446-rcc"
OBJECT_DECLARE_SIMPLE_TYPE(STM32F446RccState, STM32F446_RCC)

struct STM32F446State;

struct STM32F446RccState {
    /* <private> */
    SysBusDevice parent_obj;

    struct STM32F446State *soc;

    /* <public> */
    MemoryRegion mmio;

	uint32_t cr;
	uint32_t pllcfgr;
	uint32_t cfgr;
	uint32_t cir;
	uint32_t ahb1rstr;
	uint32_t ahb2rstr;
	uint32_t ahb3rstr;
	uint32_t apb1rstr;
	uint32_t apb2rstr;
	uint32_t ahb1enr;
	uint32_t ahb2enr;
	uint32_t ahb3enr;
	uint32_t apb1enr;
	uint32_t apb2enr;
	uint32_t ahb1lpenr;
	uint32_t ahb2lpenr;
	uint32_t ahb3lpenr;
	uint32_t apb1lpenr;
	uint32_t apb2lpenr;
	uint32_t bdcr;
	uint32_t csr;
	uint32_t sscgr;
	uint32_t plli2scfgr;
	uint32_t pllsaicfgr;
	uint32_t dckcfgr;
	uint32_t ckgatenr;
	uint32_t dckcfgr2;
};

#endif /* HW_STM32F446_RCC_H */

