/*
 * STM32F2XX FLASH
 *
 * Copyright (c) 2023 Zoltan Mihaly <>
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

#ifndef HW_STM32F4XX_FLASH_H
#define HW_STM32F4XX_FLASH_H

#include "hw/sysbus.h"
#include "qom/object.h"

#define SMT32F4XX_FLASH_ACR  0x00
#define SMT32F4XX_FLASH_KEYR     0x04
#define SMT32F4XX_FLASH_OPTKEYR 0x08
#define SMT32F4XX_FLASH_SR 0x0C
#define SMT32F4XX_FLASH_CR 0x10
#define SMT32F4XX_FLASH_OPTCR 0x14

#define SMT32F4XX_FLASH_KEY1	0x45670123
#define SMT32F4XX_FLASH_KEY2	0xCDEF89AB
#define SMT32F4XX_FLASH_OPTKEY1 0x08192A3B
#define SMT32F4XX_FLASH_OPTKEY2 0x4C5D6E7F

/*
#define STM32_FLASH_SR_BSY		(1 << 16)

#define STM32_FLASH_CR_PG		(1 << 0)
#define STM32_FLASH_CR_SER		(1 << 1)
#define STM32_FLASH_CR_STRT		(1 << 16)
#define STM32_FLASH_CR_LOCK		(1 << 31)
#define STM32_FLASH_CR_SNB_OFFSET	3
#define STM32_FLASH_CR_SNB_MASK		(15 << STM32_FLASH_CR_SNB_OFFSET)
*/

#define TYPE_STM32F4XX_FLASH "stm32f4xx-flash"
OBJECT_DECLARE_SIMPLE_TYPE(STM32F4XXFlashState, STM32F4XX_FLASH)

struct STM32F4XXFlashState {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;

	uint32_t acr;
	uint32_t keyr;
	uint32_t optkeyr;
	uint32_t sr;
	uint32_t cr;
	uint32_t optcr;
};

#endif /* HW_STM32F4XX_FLASH_H */

