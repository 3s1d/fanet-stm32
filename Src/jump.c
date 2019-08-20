/*
 * jump.c
 *
 *  Created on: May 13, 2017
 *      Author: sid
 */

#include "stm32l4xx_hal.h"

/*
 See comments below, it follows the same principles.
 */
void jump_app(void* startAddr)
{
	uint32_t* address = (uint32_t*) startAddr;
	void (*entryAddress)(void) = ((void (*)(void)) *(address + 1));

	HAL_RCC_DeInit();
	HAL_DeInit();
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;

	__HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();
	__set_MSP(*address);

	entryAddress();
}

/*
 This fowling nasty looking code is used to call the MCU's bootloader
 Some explanation: the so called "System memory" starts at 0x1FFF0000. It contains
 the MCU's bootloader code.
 At this address, the first 4 bytes is the stack pointer (look in the current
 file, __initial_sp is at the beginning).
 Then, the 4 next bytes are a pointer to the Reset Handler.
 !The device should be barely set up when calling this function!
 */
void jump_mcu_bootloader(void)
{
	uint32_t* bootloaderAddress = (uint32_t*) 0x1fff0000;
	void (*SysMemBootJump)(void) = (void (*)(void)) *(bootloaderAddress + 1);

	__HAL_FLASH_INSTRUCTION_CACHE_DISABLE();
	__HAL_FLASH_DATA_CACHE_DISABLE();
	__HAL_FLASH_PREFETCH_BUFFER_DISABLE();

	HAL_RCC_DeInit();
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;

	SYSCFG->MEMRMP = SYSCFG_MEMRMP_MEM_MODE_0;

	__disable_irq();
	__set_MSP(*bootloaderAddress);

	SysMemBootJump();
}
