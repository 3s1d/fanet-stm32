/*
 * bt_upd.c
 *
 *  Created on: 19 Aug 2019
 *      Author: sid
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "stm32l4xx.h"
#include "stm32l4xx_hal_uart.h"

#include "config.h"

#ifdef UPDATE_BOOTLOADER

#include "xloader.h"
#include "bld_upd.h"

/* inform user */
void bld_print(UART_HandleTypeDef *uart, const char *str)
{
	if(uart == NULL || str == NULL)
		return;

	while(HAL_UART_Transmit_IT(uart, (uint8_t *)str, strlen(str)) == HAL_BUSY);
	while(uart->gState != (__IO HAL_UART_StateTypeDef) HAL_UART_STATE_READY);
}

/* bootloader differs? */
bool bld_different(void)
{
	__IO uint8_t *flash = (__IO uint8_t*) HEX2Cxloader_START;

	for(uint32_t i=0; i<HEX2Cxloader_SIZE; i++, flash++)
	{
		/* image out of range */
		if((uint32_t)flash < FLASH_BASE || (uint32_t)flash > (FLASH_BASE + BLD_SIZE))
			return false;

		/* check */
		if(*flash != xloader_bin[i])
			return true;
	}

	return false;
}

/* flash included firmware */
//note: this HAS to work!
void bld_install(void)
{
	/* image too big */
	if(HEX2Cxloader_START + HEX2Cxloader_SIZE >= FLASH_BASE + BLD_SIZE)
		return;

	HAL_FLASH_Unlock();
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

	/* erase */
	FLASH_EraseInitTypeDef eraseInit = {0};
	eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
	eraseInit.Banks = FLASH_BANK_1;
	eraseInit.Page = 0;
	eraseInit.NbPages = (HEX2Cxloader_SIZE/FLASH_PAGE_SIZE)+1;
	uint32_t sectorError = 0;
	HAL_FLASHEx_Erase(&eraseInit, &sectorError);
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);					//error will occur a blank device


	/* write */
	for(uint32_t fromIdx=0, toAddr=HEX2Cxloader_START; fromIdx<HEX2Cxloader_SIZE; fromIdx+=8, toAddr+=8)
	{
		if(fromIdx+8 <= HEX2Cxloader_SIZE)
		{
			/* direct flash2flash */
			HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, toAddr, *((uint64_t *) &xloader_bin[fromIdx]));
		}
		else
		{
			/* fill missing bytes w/ 0xFF */
			uint8_t buf[8]  __attribute__ ((aligned (64)));
			uint32_t missing = fromIdx+8 - HEX2Cxloader_SIZE;
			//printf("missing: %lu\n", missing);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
			memcpy(buf, (uint8_t *) &xloader_bin[fromIdx], 8-missing);
#pragma GCC diagnostic pop
			memset(&buf[8-missing], 0xFF, missing);

			HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, toAddr, *((uint64_t *)buf));
		}
	}

	HAL_FLASH_Lock();

}

bool bootloader_update(UART_HandleTypeDef *uart)
{
	/* check */
	//note: duration 10ms
	if(bld_different() == false)
		return false;

	/* update */
	//note: duration 230ms
	bld_install();

	/* notify user */
	bld_print(uart, "#BLD MSG,1,updated\n");
	return true;
}

#endif
