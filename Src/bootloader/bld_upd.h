/*
 * bt_upd.h
 *
 *  Created on: 19 Aug 2019
 *      Author: sid
 */

#ifndef BOOTLOADER_BLD_UPD_H_
#define BOOTLOADER_BLD_UPD_H_

#define BLD_SIZE			0x00010000		//64kb

bool bootloader_update(UART_HandleTypeDef *uart);


#endif /* BOOTLOADER_BLD_UPD_H_ */
